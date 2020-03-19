//
// file:			doocs_rpc_xdr_float.c
//
#include <rpc/wrpc_first_com_include.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/clnt.h>
#include <rpc/svc.h>
#include <rpc/pmap_clnt.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <memory.h>
#include <time.h>
#include <rpc/doocs_rpc_unix_like_functions.h>

#define MCALL_MSG_SIZE 24

#define	bzero(__a_s__,__a_size__)			memset((__a_s__),0,(__a_size__))
#define bcopy(__a_src__,__a_dst__,__a_n__)	memmove((__a_dst__),(__a_src__),(__a_n__))


static int
readtcp(register struct ct_data* ct, caddr_t buf, register int len);

static int
writetcp(struct ct_data* ct, caddr_t buf, int len);


MINI_XDR_BEGIN_C_DECLS


static enum clnt_stat	clnttcp_call(CLIENT*, u_long, xdrproc_t, caddr_t, xdrproc_t, caddr_t, struct timeval);
static void		clnttcp_abort(CLIENT*);
static void		clnttcp_geterr(CLIENT*, struct rpc_err*);
static bool_t		clnttcp_freeres(CLIENT*, xdrproc_t, caddr_t);
static bool_t           clnttcp_control(CLIENT*, u_int, char*);
static void		clnttcp_destroy(CLIENT*);

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

static struct clnt_ops tcp_ops = {
	clnttcp_call,
	clnttcp_abort,
	clnttcp_geterr,
	clnttcp_freeres,
	clnttcp_destroy,
	clnttcp_control
};


/*
 * Generic client creation: takes (hostname, program-number, protocol) and
 * returns client handle. Default options are set, which the user can 
 * change using the rpc equivalent of ioctl()'s.
 */
MINI_XDR_EXPORT_FNL
CLIENT *
clnt_create(hostname, prog, vers, proto)
	char *hostname;
	unsigned prog;
	unsigned vers;
	char *proto;
{
	struct hostent *h;
	struct protoent *p;
	struct sockaddr_in sin;
	int sock;
	struct timeval tv;
	CLIENT *client;

#pragma warning (push)
#pragma warning (disable:4996)
	h = gethostbyname(hostname);
#pragma warning (pop)
	if (h == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNHOST;
		return (NULL);
	}
	if (h->h_addrtype != AF_INET) {
		/*
		 * Only support INET for now
		 */
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
#ifdef _WIN32
		rpc_createerr.cf_error.re_errno = WSAEAFNOSUPPORT; 
#else
		rpc_createerr.cf_error.re_errno = EAFNOSUPPORT; 
#endif
		return (NULL);
	}
	sin.sin_family = h->h_addrtype;
	sin.sin_port = 0;
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	bcopy(h->h_addr, (char*)&sin.sin_addr, h->h_length);
	p = getprotobyname(proto);
	if (p == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
#ifdef _WIN32
		rpc_createerr.cf_error.re_errno = WSAEPFNOSUPPORT;
#else
		rpc_createerr.cf_error.re_errno = EPFNOSUPPORT;
#endif 
		return (NULL);
	}
	sock = RPC_ANYSOCK;
	switch (p->p_proto) {
	case IPPROTO_UDP:
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		client = clntudp_create(&sin, prog, vers, tv, &sock);
		if (client == NULL) {
			return (NULL);
		}
		tv.tv_sec = 25;
		clnt_control(client, CLSET_TIMEOUT, &tv);
		break;
	case IPPROTO_TCP:
		client = clnttcp_create(&sin, prog, vers, &sock, 0, 0);
		if (client == NULL) {
			return (NULL);
		}
		tv.tv_sec = 25;
		tv.tv_usec = 0;
		clnt_control(client, CLSET_TIMEOUT, &tv);
		break;
	default:
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
#ifdef _WIN32
		rpc_createerr.cf_error.re_errno = WSAEPFNOSUPPORT; 
#else
		rpc_createerr.cf_error.re_errno = EPFNOSUPPORT; 
#endif
		return (NULL);
	}
	return (client);
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
MINI_XDR_EXPORT_FNL
CLIENT *
clnttcp_create(raddr, prog, vers, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long prog;
	u_long vers;
	register int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *h;
	register struct ct_data *ct = NULL;
	struct timeval now;
	struct rpc_msg call_msg;

	h  = (CLIENT *)mem_alloc(sizeof(*h));
	if (h == NULL) {
		(void)fprintf(stderr, "clnttcp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
		goto fooy;
	}
	ct = (struct ct_data *)mem_alloc(sizeof(*ct));
	if (ct == NULL) {
		(void)fprintf(stderr, "clnttcp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
		goto fooy;
	}

	/*
	 * If no port number given ask the pmap for one
	 */
	if (raddr->sin_port == 0) {
		u_short port;
		if ((port = pmap_getport(raddr, prog, vers, IPPROTO_TCP)) == 0) {
			mem_free((caddr_t)ct, sizeof(struct ct_data));
			mem_free((caddr_t)h, sizeof(CLIENT));
			return ((CLIENT *)NULL);
		}
		raddr->sin_port = htons(port);
	}

	/*
	 * If no socket given, open one
	 */
#ifdef _WIN32
	if (*sockp == INVALID_SOCKET) {
		struct linger slinger;
		
		*sockp = (int)socket(AF_INET, SOCK_STREAM, 0);
		bindresvport(*sockp, (struct sockaddr_in *)0);

		slinger.l_onoff = 1;
		slinger.l_linger = 0;
		setsockopt(*sockp, SOL_SOCKET, SO_LINGER, (char*)&slinger, sizeof(struct linger));

		if ((*sockp == INVALID_SOCKET)
		    || (connect(*sockp, (struct sockaddr *)raddr,
		    sizeof(*raddr)) < 0)) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = WSAGetLastError();
			(void)closesocket(*sockp);
#else
	if (*sockp < 0) {
		*sockp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		(void)bindresvport(*sockp, (struct sockaddr_in *)0);
		if ((*sockp < 0)
		    || (connect(*sockp, (struct sockaddr *)raddr,
		    sizeof(*raddr)) < 0)) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno;
			(void)close(*sockp);
#endif
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
	call_msg.rm_xid = _getpid() ^ now.tv_sec ^ now.tv_usec;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = prog;
	call_msg.rm_call.cb_vers = vers;

	/*
	 * pre-serialize the staic part of the call msg and stash it away
	 */
	xdrmem_create(&(ct->ct_xdrs), ct->ct_mcall, MCALL_MSG_SIZE,
	    XDR_ENCODE);
	if (! xdr_callhdr(&(ct->ct_xdrs), &call_msg)) {
		if (ct->ct_closeit) {
#ifdef _WIN32
			(void)closesocket(*sockp);
#else
			(void)close(*sockp);
#endif
		}
		goto fooy;
	}
	ct->ct_mpos = XDR_GETPOS(&(ct->ct_xdrs));
	XDR_DESTROY(&(ct->ct_xdrs));

	/*
	 * Create a client handle which uses xdrrec for serialization
	 * and authnone for authentication.
	 */
	xdrrec_create(&(ct->ct_xdrs), sendsz, recvsz,
	    (caddr_t)ct, readtcp, writetcp);
	h->cl_ops = &tcp_ops;
	h->cl_private = (caddr_t) ct;
	h->cl_auth = authnone_create();
	return (h);

fooy:
	/*
	 * Something goofed, free stuff and barf
	 */
	mem_free((caddr_t)ct, sizeof(struct ct_data));
	mem_free((caddr_t)h, sizeof(CLIENT));
	return ((CLIENT *)NULL);
}


//CLIENT *, u_long, xdrproc_t, caddr_t, xdrproc_t, caddr_t, struct timeval
static enum clnt_stat
clnttcp_call(h, proc, xdr_args, args_ptr, xdr_results, results_ptr, timeout)
	CLIENT *h;
	u_long proc;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
	xdrproc_t xdr_results;
	caddr_t results_ptr;
	struct timeval timeout;
{
	register struct ct_data *ct = (struct ct_data *) h->cl_private;
	register XDR *xdrs = &(ct->ct_xdrs);
	struct rpc_msg reply_msg;
	u_long x_id;
	u_long *msg_x_id = (u_long *)(ct->ct_mcall);	/* yuk */
	register bool_t shipnow;
	int refreshes = 2;

	if (!ct->ct_waitset) {
		ct->ct_wait = timeout;
	}

	shipnow =
	    (xdr_results == (xdrproc_t)0 && timeout.tv_sec == 0
	    && timeout.tv_usec == 0) ? FALSE : TRUE;

call_again:
	xdrs->x_op = XDR_ENCODE;
	ct->ct_error.re_status = RPC_SUCCESS;
	x_id = ntohl(--(*msg_x_id));
	if ((! XDR_PUTBYTES(xdrs, ct->ct_mcall, ct->ct_mpos)) ||
	    (! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xdr_args)(xdrs, args_ptr))) {
		if (ct->ct_error.re_status == RPC_SUCCESS)
			ct->ct_error.re_status = RPC_CANTENCODEARGS;
		(void)xdrrec_endofrecord(xdrs, TRUE);
		return (ct->ct_error.re_status);
	}
	if (! xdrrec_endofrecord(xdrs, shipnow))
		return (ct->ct_error.re_status = RPC_CANTSEND);
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
		reply_msg.acpted_rply.ar_results.proc = xdr_void;
		if (! xdrrec_skiprecord(xdrs))
			return (ct->ct_error.re_status);
		/* now decode and validate the response header */
		if (! xdr_replymsg(xdrs, &reply_msg)) {
			if (ct->ct_error.re_status == RPC_SUCCESS)
				continue;
			return (ct->ct_error.re_status);
		}
		if (reply_msg.rm_xid == x_id)
			break;
	}

	/*
	 * process header
	 */
	_seterr_reply(&reply_msg, &(ct->ct_error));
	if (ct->ct_error.re_status == RPC_SUCCESS) {
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
			(void)xdr_opaque_auth(xdrs, &(reply_msg.acpted_rply.ar_verf));
		}
	}  /* end successful completion */
	else {
		/* maybe our credentials need to be refreshed ... */
		if (refreshes-- && AUTH_REFRESH(h->cl_auth))
			goto call_again;
	}  /* end of unsuccessful completion */
	return (ct->ct_error.re_status);
}

static void
clnttcp_geterr(h, errp)
	CLIENT *h;
	struct rpc_err *errp;
{
	register struct ct_data *ct =
	    (struct ct_data *) h->cl_private;

	*errp = ct->ct_error;
}

static bool_t
clnttcp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct ct_data *ct = (struct ct_data *)cl->cl_private;
	register XDR *xdrs = &(ct->ct_xdrs);

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
	register struct ct_data *ct = (struct ct_data *)cl->cl_private;

	switch (request) {
	case CLSET_TIMEOUT:
		ct->ct_wait = *(struct timeval *)info;
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
	register struct ct_data *ct =
	    (struct ct_data *) h->cl_private;

	if (ct->ct_closeit) {
#ifdef WIN32
		(void)closesocket(ct->ct_sock);
#else
		(void)close(ct->ct_sock);
#endif
	}
	XDR_DESTROY(&(ct->ct_xdrs));
	mem_free((caddr_t)ct, sizeof(struct ct_data));
	mem_free((caddr_t)h, sizeof(CLIENT));
}


MINI_XDR_END_C_DECLS


/*
 * Interface between xdr serializer and tcp connection.
 * Behaves like the system calls, read & write, but keeps some error state
 * around for the rpc level.
 */
static int
readtcp(ct, buf, len)
	register struct ct_data *ct;
	caddr_t buf;
	register int len;
{
#ifdef FD_SETSIZE
	fd_set mask;
	fd_set readfds;

	if (len == 0)
		return (0);
	FD_ZERO(&mask);
	FD_SET(ct->ct_sock, &mask);
#else
	register int mask = 1 << (ct->ct_sock);
	int readfds;

	if (len == 0)
		return (0);

#endif /* def FD_SETSIZE */
	while (TRUE) {
		readfds = mask;
#ifdef _WIN32
		switch (select(0 /* unused in winsock */, &readfds, NULL, NULL,&(ct->ct_wait))) {
		case 0:
			ct->ct_error.re_status = RPC_TIMEDOUT;
			return (-1);

		case -1:
			if (WSAGetLastError() == EINTR)
				continue;
			ct->ct_error.re_status = RPC_CANTRECV;
			ct->ct_error.re_errno = WSAGetLastError();
			return (-1);
		}
		break;
	}
	switch (len = recv(ct->ct_sock, buf, len, 0)) {

	case 0:
		/* premature eof */
		ct->ct_error.re_errno = WSAECONNRESET;
		ct->ct_error.re_status = RPC_CANTRECV;
		len = -1;  /* it's really an error */
		break;

	case -1:
		ct->ct_error.re_errno = WSAGetLastError();
		ct->ct_error.re_status = RPC_CANTRECV;
		break;
	}
	return (len);
#else
		switch (select(_rpc_dtablesize(), &readfds, (int*)NULL, (int*)NULL,
			       &(ct->ct_wait))) {
		case 0:
			ct->ct_error.re_status = RPC_TIMEDOUT;
			return (-1);

		case -1:
			if (errno == EINTR)
				continue;
			ct->ct_error.re_status = RPC_CANTRECV;
			ct->ct_error.re_errno = errno;
			return (-1);
		}
		break;
	}
	switch (len = read(ct->ct_sock, buf, len)) {

	case 0:
		/* premature eof */
		ct->ct_error.re_errno = ECONNRESET;
		ct->ct_error.re_status = RPC_CANTRECV;
		len = -1;  /* it's really an error */
		break;

	case -1:
		ct->ct_error.re_errno = errno;
		ct->ct_error.re_status = RPC_CANTRECV;
		break;
	}
	return (len);
#endif
}


static int
writetcp(ct, buf, len)
	struct ct_data *ct;
	caddr_t buf;
	int len;
{
	register int i, cnt;

	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
#ifdef _WIN32
		if ((i = send(ct->ct_sock, buf, cnt, 0)) == -1) {
			ct->ct_error.re_errno = WSAGetLastError();
#else
		if ((i = write(ct->ct_sock, buf, cnt)) == -1) {
			ct->ct_error.re_errno = errno;
#endif
			ct->ct_error.re_status = RPC_CANTSEND;
			return (-1);
		}
	}
	return (len);
}
