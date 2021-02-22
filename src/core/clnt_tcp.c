/*********************************************************************
 * RPC for the Windows NT Operating System
 * 1993 by Martin F. Gergeleit
 * Users may use, copy or modify Sun RPC for the Windows NT Operating 
 * System according to the Sun copyright below.
 *
 * RPC for the Windows NT Operating System COMES WITH ABSOLUTELY NO 
 * WARRANTY, NOR WILL I BE LIABLE FOR ANY DAMAGES INCURRED FROM THE 
 * USE OF. USE ENTIRELY AT YOUR OWN RISK!!!
 *********************************************************************/

/* @(#)clnt_tcp.c	2.2 88/08/01 4.0 RPCSRC */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)clnt_tcp.c 1.37 87/10/05 Copyr 1984 Sun Micro";
#endif

/*
 * clnt_tcp.c, Implements a TCP/IP based, client side RPC.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * TCP based RPC supports 'batched calls'.
 * A sequence of calls may be batched-up in a send buffer.  The rpc call
 * return immediately to the client even though the call was not necessarily
 * sent.  The batching occurs if the results' xdr routine is NULL (0) AND
 * the rpc timeout value is zero (see clnt.h, rpc).
 *
 * Clients should NOT casually batch calls that in fact return results; that is,
 * the server side should be aware that a call is batched and not produce any
 * return message.  Batched calls that produce many result messages can
 * deadlock (netlock) the client and the server....
 *
 * Now go hang yourself.
 */

#include <rpc/wrpc_first_com_include.h>
#include "xdr_rpc_debug.h"
#include <rpc/pmap_clnt.h>
#include <errno.h>
#include <stdio.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/rpc_unix_like_functions.h>
#include "mini_xdr_rpc_src_private.h"

#ifdef _WIN32
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

#define MCALL_MSG_SIZE	24

MINI_XDR_BEGIN_C_DECLS

struct ct_data;

static int	readtcp(XDR_RPC_REGISTER struct ct_data* ct, caddr_t buf,XDR_RPC_REGISTER int len);
static int	writetcp(struct ct_data* ct,caddr_t buf,int len);

static enum clnt_stat	clnttcp_call(CLIENT *, u_long, xdrproc_t, caddr_t, xdrproc_t, caddr_t, struct timeval);
static void		clnttcp_abort(CLIENT *);
static void		clnttcp_geterr(CLIENT *, struct rpc_err *);
static bool_t		clnttcp_freeres(CLIENT*, xdrproc_t, caddr_t);
static bool_t           clnttcp_control(CLIENT*, u_int, char*);
static void		clnttcp_destroy(CLIENT*);

static struct clnt_ops tcp_ops = {
	clnttcp_call,
	clnttcp_abort,
	clnttcp_geterr,
	clnttcp_freeres,
	clnttcp_destroy,
	clnttcp_control
};

struct ct_data {
	int		ct_sock;
	bool_t		ct_closeit;
	struct timeval	ct_wait;
	bool_t          ct_waitset;       /* wait set by clnt_control? */
	struct sockaddr_in ct_addr;
	struct rpc_err	ct_error;
	char		ct_mcall[MCALL_MSG_SIZE];	/* marshalled callmsg */
	u_int		ct_mpos;			/* pos after marshal */
	XDR		ct_xdrs;
};

MINI_XDR_EXPORT
int set_socket_timeout(xdrRpcSock a_socket, const struct timeval * a_pTimeout)
{
	char* pInput;
	socklen_t nInputLen;
	
#ifdef _WIN32
	DWORD dwTimeout;
	if(a_pTimeout) {
		dwTimeout = a_pTimeout->tv_sec * 1000 + a_pTimeout->tv_usec / 1000 ;
	}
	else {
		dwTimeout = INFINITE;
	}
	pInput = (char*)&dwTimeout;
	nInputLen = sizeof(DWORD);	
#else
	struct timeval tv;
	if(a_pTimeout) {tv=*a_pTimeout;}
	else {
		tv.tv_sec = 21474836;
		tv.tv_usec = 1000;
	}
	pInput = (char*)&tv;
	nInputLen = sizeof(struct timeval);
#endif

	if (setsockopt(a_socket,SOL_SOCKET,SO_RCVTIMEO,pInput,nInputLen) < 0){return -1;}
	
	return 0;	
}


/*
 * Create a client handle for a tcp/ip connection.
 * If *sockp<0, *sockp is set to a newly created TCP socket and it is
 * connected to raddr.  If *sockp non-negative then
 * raddr is ignored.  The rpc/tcp package does buffering
 * similar to stdio, so the client must pick send and receive buffer sizes,];
 * 0 => use the default.
 * If raddr->sin_port is 0, then a binder on the remote machine is
 * consulted for the right port number.
 * NB: *sockp is copied into a private area.
 * NB: It is the clients responsibility to close *sockp.
 * NB: The rpch->cl_auth is set null authentication.  Caller may wish to set this
 * something more useful.
 */
MINI_XDR_EXPORT
CLIENT *
clnttcp_create(raddr, prog, vers, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long prog;
	u_long vers;
	XDR_RPC_REGISTER int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *h;
	XDR_RPC_REGISTER struct ct_data *ct = NULL;
	struct timeval now;
	struct rpc_msg call_msg;
    
    XDR_RPC_DEBUG("*sockp=%d",(int)(*sockp));

	h  = (CLIENT *)mem_alloc(sizeof(*h));
	if (!h) {
		XDR_RPC_ERR("out of memory");
		goto fooy;
	}
	ct = (struct ct_data *)mem_alloc(sizeof(*ct));
	if (!ct) {
		XDR_RPC_ERR("out of memory");
		goto fooy;
	}
    
    XDR_RPC_DEBUG("*sockp=%d,raddr->sin_port=%d",(int)(*sockp),(int)raddr->sin_port);

	/*
	 * If no port number given ask the pmap for one
	 */
	if (raddr->sin_port == 0) {
		u_short port;
		// todo: in the case of windows maybe change below will not work
		if((port = pmap_getport(raddr, prog, vers, IPPROTO_TCP)) == 0) {
			mem_free(ct, sizeof(struct ct_data));
			mem_free(h, sizeof(CLIENT));
			return ((CLIENT *)NULL);
		}
		raddr->sin_port = htons(port);
	}
    
    XDR_RPC_DEBUG("*sockp=%d,raddr->sin_port=%d",(int)(*sockp),(int)raddr->sin_port);

	/*
	 * If no socket given, open one
	 */
	if (*sockp < 0) {
		*sockp = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		(void)bindresvport(*sockp, (struct sockaddr_in *)0);
		if ((*sockp < 0)|| (connect(*sockp, (struct sockaddr *)raddr,sizeof(*raddr)) < 0)) {
			(void)closesocket(*sockp);	
			goto fooy;
		}
		ct->ct_closeit = TRUE;
	} else {
		ct->ct_closeit = FALSE;
	}

	/*
	 * Set up private data struct
	 */
	ct->ct_sock = *sockp;
	ct->ct_wait.tv_usec = 0;
	ct->ct_waitset = FALSE;
	ct->ct_addr = *raddr;

	/*
	 * Initialize call message
	 */
	(void)gettimeofday(&now, (struct timezone *)0);
	XDR_RPC_DEBUG("now.tv_sec=%d,now.tv_usec=%d",(int)now.tv_sec,(int)now.tv_usec);
	call_msg.rm_xid = getpid() ^ now.tv_sec ^ now.tv_usec;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = prog;
	call_msg.rm_call.cb_vers = vers;

	/*
	 * pre-serialize the staic part of the call msg and stash it away
	 */
	XDR_RPC_DEBUG("now.tv_sec=%d,now.tv_usec=%d",(int)now.tv_sec,(int)now.tv_usec);
	xdrmem_create(&(ct->ct_xdrs), ct->ct_mcall, MCALL_MSG_SIZE,XDR_ENCODE);
	XDR_RPC_DEBUG("now.tv_sec=%d,now.tv_usec=%d",(int)now.tv_sec,(int)now.tv_usec);
	if (! xdr_callhdr(&(ct->ct_xdrs), &call_msg)) {
		XDR_RPC_DEBUG("now.tv_sec=%d,now.tv_usec=%d\n",(int)now.tv_sec,(int)now.tv_usec);
		if (ct->ct_closeit) {
			(void)closesocket(*sockp);
		}
		goto fooy;
	}
	XDR_RPC_DEBUG("now.tv_sec=%d,now.tv_usec=%d\n",(int)now.tv_sec,(int)now.tv_usec);
	ct->ct_mpos = XDR_GETPOS(&(ct->ct_xdrs));
	XDR_DESTROY(&(ct->ct_xdrs));

	/*
	 * Create a client handle which uses xdrrec for serialization
	 * and authnone for authentication.
	 */
	XDR_RPC_DEBUG("now.tv_sec=%d,now.tv_usec=%d\n",(int)now.tv_sec,(int)now.tv_usec);
	xdrrec_create(&(ct->ct_xdrs), sendsz, recvsz,(caddr_t)ct, readtcp, writetcp);
	XDR_RPC_DEBUG("now.tv_sec=%d,now.tv_usec=%d\n",(int)now.tv_sec,(int)now.tv_usec);
	h->cl_ops = &tcp_ops;
	h->cl_private = (caddr_t) ct;
	h->cl_auth = authnone_create();
	XDR_RPC_DEBUG("now.tv_sec=%d,now.tv_usec=%d\n",(int)now.tv_sec,(int)now.tv_usec);
	return (h);

fooy:
	/*
	 * Something goofed, free stuff and barf
	 */
	mem_free(ct, sizeof(struct ct_data));
	mem_free(h, sizeof(CLIENT));
	return ((CLIENT *)NULL);
}

//#include <trace.h>
//#define __cplusplus
#ifdef _WIN32
#define pthread_self_emscr GetCurrentThreadId
#else
#include <pthread.h>
#ifdef EMSCRIPTEN
pthread_t pthread_self_emscr(void);
#else
#define pthread_self_emscr	pthread_self
#endif
#endif

//CLIENT *, u_long, xdrproc_t, caddr_t, xdrproc_t, caddr_t, struct timeval
// enum clnt_stat	(*cl_call)(struct CLIENTstruct *, u_long, xdrproc_t, caddr_t, xdrproc_t, caddr_t, struct timeval);	/* call remote procedure */
static enum clnt_stat
clnttcp_call(h, proc, xdr_args, args_ptr, xdr_results, results_ptr, timeout)
	struct CLIENTstruct *h;
	u_long proc;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
	xdrproc_t xdr_results;
	caddr_t results_ptr;
	struct timeval timeout;
{
	XDR_RPC_REGISTER struct ct_data *ct = (struct ct_data *) h->cl_private;
	XDR_RPC_REGISTER XDR *xdrs = &(ct->ct_xdrs);
	struct rpc_msg reply_msg;
	u_long x_id;
	u_long *msg_x_id = (u_long *)(ct->ct_mcall);	/* yuk */
	XDR_RPC_REGISTER bool_t shipnow;
	int refreshes = 2;
		
	XDR_RPC_DEBUG("proc=%d",(int)proc);

	if (!ct->ct_waitset) {
		ct->ct_wait = timeout;
		if(ct->ct_sock>=0) {
			set_socket_timeout(ct->ct_sock,&timeout);
		}
	}
	
	XDR_RPC_DEBUG("  ");

	shipnow =
	    (xdr_results == (xdrproc_t)0 && timeout.tv_sec == 0
	    && timeout.tv_usec == 0) ? FALSE : TRUE;

call_again:
	xdrs->x_op = XDR_ENCODE;
	ct->ct_error.re_status = RPC_SUCCESS;
	x_id = ntohl(--(*msg_x_id));
	XDR_RPC_DEBUG("  ");
	
		
	if ((! XDR_PUTBYTES(xdrs, ct->ct_mcall, ct->ct_mpos)) ||
	    (! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xdr_args)(xdrs, args_ptr))) {
		XDR_RPC_DEBUG("  ");
		if (ct->ct_error.re_status == RPC_SUCCESS)
			ct->ct_error.re_status = RPC_CANTENCODEARGS;
		XDR_RPC_DEBUG("xdr_args=%p, gettid()=%d",(void*)xdr_args,(int)pthread_self_emscr());
		(void)xdrrec_endofrecord(xdrs, TRUE);
		XDR_RPC_DEBUG("xdr_args=%p, gettid()=%d\n",(void*)xdr_args,(int)pthread_self_emscr());
		return (ct->ct_error.re_status);
	}
	XDR_RPC_DEBUG("  ");
	if (! xdrrec_endofrecord(xdrs, shipnow)){
		XDR_RPC_DEBUG("  ");
		return (ct->ct_error.re_status = RPC_CANTSEND);
	}
	XDR_RPC_DEBUG("shipnow=%d",shipnow);
	if (! shipnow)
		return (RPC_SUCCESS);
	/*
	 * Hack to provide rpc-based message passing
	 */
	if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
		return(ct->ct_error.re_status = RPC_TIMEDOUT);
	}


	/*
	 * Keep receiving until we get a valid transaction id
	 */
	xdrs->x_op = XDR_DECODE;
	while (TRUE) {
		reply_msg.acpted_rply.ar_verf = _null_auth;
		reply_msg.acpted_rply.ar_results.where = NULL;
		reply_msg.acpted_rply.ar_results.proc = &xdr_void;
		XDR_RPC_DEBUG("  ");
		if (! xdrrec_skiprecord(xdrs)){
			XDR_RPC_DEBUG("  ");
			return (ct->ct_error.re_status);
		}
		XDR_RPC_DEBUG("  ");
		/* now decode and validate the response header */
		if (! xdr_replymsg(xdrs, &reply_msg)) {
			XDR_RPC_DEBUG("  ");
			if (ct->ct_error.re_status == RPC_SUCCESS)
				continue;
			return (ct->ct_error.re_status);
		}
		XDR_RPC_DEBUG("  ");
		if (reply_msg.rm_xid == x_id)
			break;
	}

	XDR_RPC_DEBUG("  ");
	/*
	 * process header
	 */
	_seterr_reply(&reply_msg, &(ct->ct_error));
	XDR_RPC_DEBUG("  ");
	if (ct->ct_error.re_status == RPC_SUCCESS) {
		XDR_RPC_DEBUG("  ");
		if (! AUTH_VALIDATE(h->cl_auth, &reply_msg.acpted_rply.ar_verf)) {
			ct->ct_error.re_status = RPC_AUTHERROR;
			ct->ct_error.re_why = AUTH_INVALIDRESP;
		} else if (! (*xdr_results)(xdrs, results_ptr)) {
			if (ct->ct_error.re_status == RPC_SUCCESS)
				ct->ct_error.re_status = RPC_CANTDECODERES;
		}
		/* free verifier ... */
		if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
			xdrs->x_op = XDR_FREE;
			XDR_RPC_DEBUG("  ");
			(void)xdr_opaque_auth(xdrs, &(reply_msg.acpted_rply.ar_verf));
			XDR_RPC_DEBUG("  ");
		}
	}  /* end successful completion */
	else {
		XDR_RPC_DEBUG("  ");
		/* maybe our credentials need to be refreshed ... */
		if (refreshes-- && AUTH_REFRESH(h->cl_auth))
			goto call_again;
	}  /* end of unsuccessful completion */
	XDR_RPC_DEBUG("  ");
	return (ct->ct_error.re_status);
}


static void
clnttcp_geterr(h, errp)
	CLIENT *h;
	struct rpc_err *errp;
{
	XDR_RPC_REGISTER struct ct_data *ct =
	    (struct ct_data *) h->cl_private;

	*errp = ct->ct_error;
}


static bool_t
clnttcp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	XDR_RPC_REGISTER struct ct_data *ct = (struct ct_data *)cl->cl_private;
	XDR_RPC_REGISTER XDR *xdrs = &(ct->ct_xdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}


static void
clnttcp_abort(CLIENT * c)
{
}


static bool_t
clnttcp_control(cl, request, info)
	CLIENT *cl;
	u_int request;
	char *info;
{
	XDR_RPC_REGISTER struct ct_data *ct = (struct ct_data *)cl->cl_private;

	switch (request) {
	case CLSET_TIMEOUT:
		ct->ct_wait = *(struct timeval *)info;
		if(ct->ct_sock>=0) {
			set_socket_timeout(ct->ct_sock,&ct->ct_wait);
		}
		ct->ct_waitset = TRUE;
		break;
	case CLGET_TIMEOUT:
		*(struct timeval *)info = ct->ct_wait;
		break;
	case CLGET_SERVER_ADDR:
		*(struct sockaddr_in *)info = ct->ct_addr;
		break;
	default:
		return (FALSE);
	}
	return (TRUE);
}


static void
clnttcp_destroy(h)
	CLIENT *h;
{
	XDR_RPC_REGISTER struct ct_data *ct =
	    (struct ct_data *) h->cl_private;

	if (ct->ct_closeit) {
		(void)closesocket(ct->ct_sock);
	}
	XDR_DESTROY(&(ct->ct_xdrs));
	mem_free(ct, sizeof(struct ct_data));
	mem_free(h, sizeof(CLIENT));
}

// static int	readtcp(XDR_RPC_REGISTER struct ct_data* ct, caddr_t buf,XDR_RPC_REGISTER int len);
/*
 * Interface between xdr serializer and tcp connection.
 * Behaves like the system calls, read & write, but keeps some error state
 * around for the rpc level.
 */
static int
readtcp(ct, buf, len)
	XDR_RPC_REGISTER struct ct_data *ct;
	caddr_t buf;
	XDR_RPC_REGISTER int len;
{
	XDR_RPC_DEBUG("sock=%d,len=%d",(int)ct->ct_sock,len);
	len = (int)recv(ct->ct_sock, buf, (sndrcv_size_t)len, 0);
	XDR_RPC_DEBUG("sock=%d,len=%d\n",(int)ct->ct_sock,len);
	
	if(len==SOCKET_ERROR) {
		ct->ct_error.re_status = RPC_CANTRECV;
		ct->ct_error.re_errno = WSAGetLastError();
	}
	
	return len;	
}


static int
writetcp(ct, buf, len)
	struct ct_data *ct;
	caddr_t buf;
	int len;
{
	XDR_RPC_REGISTER int i, cnt;

	XDR_RPC_DEBUG("  ");

	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
		if ((i = send(ct->ct_sock, buf, cnt, 0)) == SOCKET_ERROR) {
			ct->ct_error.re_errno = WSAGetLastError();
			ct->ct_error.re_status = RPC_CANTSEND;
			return (-1);
		}
	}
	XDR_RPC_DEBUG("sock=%d,len=%d\n",(int)ct->ct_sock,len);
	return (len);
}

MINI_XDR_END_C_DECLS
