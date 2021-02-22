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
#include "xdr_rpc_debug.h"


MINI_XDR_BEGIN_C_DECLS

MINI_XDR_DLL_PRIVATE 
int GetHostSinAddrAndReturnProto(struct in_addr* sin_addr_p, const char* hostName, const char* protoName, const char* portAsStrOrServiceName);
MINI_XDR_DLL_PRIVATE 
int GetHostSinAddrAndReturnProto2(struct in_addr* sin_addr_p, const char* hostName, const char* protoName, int portNumber);

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
	struct sockaddr_in sin;
	int sock;
	struct timeval tv;
	CLIENT *client;
	int nProto;

	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	
	//bcopy(h->h_addr, (char*)&sin.sin_addr, h->h_length);
	//p = getprotobyname(proto);
	nProto = GetHostSinAddrAndReturnProto(&sin.sin_addr,hostname,proto,NULL);
	
	
	if (nProto<0) {
		return (NULL);
	}
	sock = RPC_ANYSOCK;
	switch (nProto) {
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
	default:
		client = clnttcp_create(&sin, prog, vers, &sock, 0, 0);
		if (client == NULL) {
			return (NULL);
		}
		tv.tv_sec = 25;
		tv.tv_usec = 0;
		clnt_control(client, CLSET_TIMEOUT, (caddr_t)&tv);
		break;
	}
	return (client);
}


MINI_XDR_DLL_PRIVATE int GetHostSinAddrAndReturnProto(struct in_addr* a_sin_addr_p,const char* a_hostName, const char* a_protoName, const char* a_portAsStrOrServiceName)
{
#ifdef _WIN32

	int nReturn = -1;
	struct protoent* prot = NULL;
	struct addrinfo* pAddrInfo = NULL;
	struct addrinfo* ptr;
	struct addrinfo hints;
	INT getAddrInfoRet;
	struct sockaddr_in* ipv4_addr_ptr;

	if(a_protoName){
		prot = getprotobyname(a_protoName);
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = 0;  // 0 means unspecified
	hints.ai_protocol = prot? prot->p_proto:0;

	getAddrInfoRet = getaddrinfo(a_hostName,a_portAsStrOrServiceName,&hints, &pAddrInfo);
	if (getAddrInfoRet) {
		goto returnPoint;
	}

	for (ptr = pAddrInfo; ptr != NULL; ptr = ptr->ai_next) {
		switch (ptr->ai_family) {
		case AF_INET:
			ipv4_addr_ptr = (struct sockaddr_in*)ptr->ai_addr;
			memcpy(a_sin_addr_p, &ipv4_addr_ptr->sin_addr,sizeof(struct in_addr));
			nReturn = hints.ai_protocol;
			goto returnPoint;
			break;
		default:
			break;
		}
	}

	nReturn = hints.ai_protocol;
returnPoint:
	if (pAddrInfo) { freeaddrinfo(pAddrInfo); }
	return nReturn;

#else   // #ifdef _WIN32

	// todo: implement case of UNIX
	(void)a_hostName;
	(void)a_sin_addr_p;
	return 0;

#endif  // #ifdef _WIN32
}


MINI_XDR_DLL_PRIVATE int GetHostSinAddrAndReturnProto2(struct in_addr* a_sin_addr_p,const char* a_hostName, const char* a_protoName,int a_portNumber)
{
	if(a_portNumber>0){
		char vcPortNumberStr[64];
		snprintf(vcPortNumberStr, 63, "%d", a_portNumber);
		return GetHostSinAddrAndReturnProto(a_sin_addr_p,a_hostName,a_protoName, vcPortNumberStr);
	}

	return GetHostSinAddrAndReturnProto(a_sin_addr_p, a_hostName, a_protoName,NULL);
}

MINI_XDR_END_C_DECLS
