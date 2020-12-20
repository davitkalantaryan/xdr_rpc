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

/* @(#)clnt_generic.c	2.2 88/08/01 4.0 RPCSRC */
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
static char sccsid[] = "@(#)clnt_generic.c 1.4 87/08/11 (C) 1987 SMI";
#endif
/*
 * Copyright (C) 1987, Sun Microsystems, Inc.
 */

#include <rpc/wrpc_first_com_include.h>
#include <errno.h>
#include <rpc/types.h>
#include <rpc/clnt.h>
#include <rpc/svc.h>
#include "mini_xdr_rpc_src_private.h"
#ifdef _WIN32
#else
#include <netdb.h>
#include <sys/socket.h>
#endif

MINI_XDR_BEGIN_C_DECLS

/*
 * Generic client creation: takes (hostname, program-number, protocol) and
 * returns client handle. Default options are set, which the user can 
 * change using the rpc equivalent of ioctl()'s.
 */
MINI_XDR_EXPORT
CLIENT *
clnt_create(hostname, prog, vers, proto)
	char *hostname;
	unsigned prog;
	unsigned vers;
	char *proto;
{
	//struct hostent *h;
	//struct protoent *p;
	struct sockaddr_in sin;
	int sock;
	struct timeval tv;
	CLIENT *client;
	struct addrinfo hints, *res=NULL;
	int nRet;
	int    p_proto = IPPROTO_TCP;

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4996)
#endif
	
	//return NULL;
	
	//h = gethostbyname(hostname);
	
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;
	nRet=getaddrinfo(hostname, NULL, &hints, &res);
	
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    
    XDR_RPC_DEBUG("file:%s,line:%d, proto=%s\n",__FILE__,__LINE__,proto?proto:"null");
	
	if(nRet || (res->ai_family!=AF_INET)) {  // todo: for the future make loop to find internet interface
		rpc_createerr.cf_stat = RPC_UNKNOWNHOST;
		return (NULL);
	}
    
#if 0
	if (!h) {
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
#endif
	
	sin = *((struct sockaddr_in *)res->ai_addr);
	freeaddrinfo(res);
	
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	
	//bzero(sin.sin_zero, sizeof(sin.sin_zero));
	//bcopy(h->h_addr, (char*)&sin.sin_addr, h->h_length);
	//sin.sin_addr = res->ai_addr->sa_data;
	
#if 0
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
    XDR_RPC_DEBUG("file:%s,line:%d, p->p_proto=%d\n",__FILE__,__LINE__,(int)p->p_proto);
#endif
	sock = RPC_ANYSOCK;
	switch (p_proto) {
	case IPPROTO_UDP:
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		client = clntudp_create(&sin, prog, vers, tv, &sock);
		if (client == NULL) {
			return (NULL);
		}
		tv.tv_sec = 25;
		clnt_control(client, CLSET_TIMEOUT, (caddr_t)&tv);
		break;
	case IPPROTO_TCP:
		client = clnttcp_create(&sin, prog, vers, &sock, 0, 0);
		XDR_RPC_DEBUG("file:%s,line:%d, proto=%s, client=%p\n",__FILE__,__LINE__,proto?proto:"null",(void*)client);
		if (client == NULL) {
			return (NULL);
		}
		tv.tv_sec = 25;
		tv.tv_usec = 0;
		clnt_control(client, CLSET_TIMEOUT, (caddr_t)&tv);
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

MINI_XDR_END_C_DECLS
