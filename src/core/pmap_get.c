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

/* @(#)pmap_getport.c	2.2 88/08/01 4.0 RPCSRC */
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
static char sccsid[] = "@(#)pmap_getport.c 1.9 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * pmap_getport.c
 * Client interface to pmap rpc service.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <rpc/wrpc_first_com_include.h>
#include "xdr_rpc_debug.h"
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#include <rpc/clnt.h>
#include "mini_xdr_rpc_src_private.h"

MINI_XDR_BEGIN_C_DECLS

static struct timeval timeout = { 5, 0 };
static struct timeval tottimeout = { 60, 0 };

MINI_XDR_EXPORT
bool_t
xdr_pmap(XDR *xdrs, void* vpregs, ...)	
{
	struct pmap *regs = (struct pmap *)vpregs;
	XDR_RPC_DEBUG(" ");
	if (xdr_u_long(xdrs, &regs->pm_prog) && 
		xdr_u_long(xdrs, &regs->pm_vers) && 
	    xdr_u_long(xdrs, &regs->pm_prot)){
		XDR_RPC_DEBUG(" ");
		return (xdr_u_long(xdrs, &regs->pm_port));
	}
	XDR_RPC_DEBUG(" ");
	return (FALSE);
}

/*
 * Find the mapped port for program,version.
 * Calls the pmap service remotely to do the lookup.
 * Returns 0 if no map exists.
 */
MINI_XDR_EXPORT
u_short
pmap_getport_udp(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_int protocol;
{
	u_short port = 0;
	int socket = -1;
	struct rpc_err aError;
	register CLIENT *client;
	struct pmap parms;
    
    XDR_RPC_DEBUG("  ");

	address->sin_port = htons(PMAPPORT);
	client = clntudp_bufcreate(address, PMAPPROG,PMAPVERS, timeout, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
    XDR_RPC_DEBUG("client=%p",(void*)client);
	if (client != (CLIENT *)NULL) {
		XDR_RPC_DEBUG("  ");
		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_prot = protocol;
		parms.pm_port = 0;  /* not needed or used */
		XDR_RPC_DEBUG("client->cl_ops->cl_call:%p,&xdr_pmap=%p\n",(void*)client->cl_ops->cl_call,(void*)&xdr_pmap);
		if (CLNT_CALL(client, PMAPPROC_GETPORT, xdr_pmap, (caddr_t)&parms,xdr_u_short, (caddr_t)&port, tottimeout) != RPC_SUCCESS){
			clnt_geterr(client, &aError);
			XDR_RPC_ERR("rpc error: %d", aError.re_errno);
		} else if (port == 0) {
			XDR_RPC_DEBUG("  ");
		}
		XDR_RPC_DEBUG("  ");
		CLNT_DESTROY(client);
		XDR_RPC_DEBUG("  ");
	}
	(void)closesocket(socket);
	address->sin_port = 0;
	XDR_RPC_DEBUG("  ");
	return (port);
}
	
	
	
/*
 * Find the mapped port for program,version.
 * Calls the pmap service remotely to do the lookup.
 * Returns 0 if no map exists.
 */
MINI_XDR_EXPORT
u_short
pmap_getport(struct sockaddr_in * address, u_long program, u_long version, u_int protocol)
{
	u_short port = 0;
	int socket = -1;
	struct rpc_err aError;
	register CLIENT *client;
	struct pmap parms;
	
	XDR_RPC_DEBUG("  ");

	address->sin_port = htons(PMAPPORT);
	client = clnttcp_create(address, PMAPPROG,PMAPVERS, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	XDR_RPC_DEBUG("client=%p",(void*)client);
	if (client != (CLIENT *)NULL) {
		XDR_RPC_DEBUG("  ");
		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_prot = protocol;
		parms.pm_port = 0;  /* not needed or used */
		if (CLNT_CALL(client, PMAPPROC_GETPORT, &xdr_pmap, (caddr_t)&parms,&xdr_u_short, (caddr_t)&port, tottimeout) != RPC_SUCCESS){
			XDR_RPC_DEBUG("  ");
			clnt_geterr(client, &aError);
			XDR_RPC_ERR("rpc failure: errorCode: %d", aError.re_errno);
		} else if (port == 0) {
			XDR_RPC_DEBUG("  ");
		}
		CLNT_DESTROY(client);
		XDR_RPC_DEBUG("  ");
	}
	(void)closesocket(socket);
	address->sin_port = 0;
	XDR_RPC_DEBUG("  ");
	return (port);
}


MINI_XDR_END_C_DECLS
