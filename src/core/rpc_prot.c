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

/* @(#)rpc_prot.c	2.3 88/08/07 4.0 RPCSRC */
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
static char sccsid[] = "@(#)rpc_prot.c 1.36 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * rpc_prot.c
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * This set of routines implements the rpc message definition,
 * its serializer and some common rpc utility routines.
 * The routines are meant for various implementations of rpc -
 * they are NOT for the rpc client or rpc service implementations!
 * Because authentication stuff is easy and is part of rpc, the opaque
 * routines are also in this program.
 */

#include <rpc/wrpc_first_com_include.h>
#include "xdr_rpc_debug.h"
#include "rpc/types.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/rpc_msg.h"
#include <rpc/clnt.h>
#include "mini_xdr_rpc_src_private.h"


/* * * * * * * * * * * * * * XDR Authentication * * * * * * * * * * * */

MINI_XDR_BEGIN_C_DECLS

/*
 * XDR an opaque authentication struct
 * (see auth.h)
 */
MINI_XDR_EXPORT
bool_t
xdr_opaque_auth(XDR_RPC_REGISTER XDR * xdrs, void* app, ...)
{
	register struct opaque_auth *ap =  (struct opaque_auth *)app;

	if (xdr_enum(xdrs, &(ap->oa_flavor)))
		return (xdr_bytes(xdrs, &ap->oa_base,
			&ap->oa_length, MAX_AUTH_BYTES));
	return (FALSE);
}

/*
 * XDR a DES block
 */
//MINI_XDR_EXPORT // no need to export
//static
//bool_t
//xdr_des_block(XDR_RPC_REGISTER XDR * xdrs, void* blkpp, ...)
//{
//	register des_block *blkp = (des_block*)blkpp;
//	return (xdr_opaque(xdrs, (caddr_t)blkp, sizeof(des_block)));
//}

/* * * * * * * * * * * * * * XDR RPC MESSAGE * * * * * * * * * * * * * * * */

/*
 * XDR the MSG_ACCEPTED part of a reply message union
 */
//MINI_XDR_EXPORT // no need to export
static
bool_t 
xdr_accepted_reply(XDR_RPC_REGISTER XDR * xdrs, void* arp, ...)
{
	register struct accepted_reply *ar = (struct accepted_reply *)arp;

	/* personalized union, rather than calling xdr_union */
	if (! xdr_opaque_auth(xdrs, &(ar->ar_verf)))
		return (FALSE);
	if (! xdr_enum(xdrs, (enum_t *)&(ar->ar_stat)))
		return (FALSE);
	switch (ar->ar_stat) {

	case SUCCESS:
		return ((*(ar->ar_results.proc))(xdrs, ar->ar_results.where));

	case PROG_MISMATCH:
		if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
			return (FALSE);
		return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
	default:
		fprintf(stderr,"%s default:\n",__FUNCTION__);
		break;
	}
	return (TRUE);  /* TRUE => open ended set of problems */
}

/*
 * XDR the MSG_DENIED part of a reply message union
 */
//MINI_XDR_EXPORT // no need to export
static
bool_t 
xdr_rejected_reply(XDR_RPC_REGISTER XDR * xdrs, void* rrp, ...)
{
	register struct rejected_reply *rr = (struct rejected_reply *)rrp;

	/* personalized union, rather than calling xdr_union */
	if (! xdr_enum(xdrs, (enum_t *)&(rr->rj_stat)))
		return (FALSE);
	switch (rr->rj_stat) {

	case RPC_MISMATCH:
		if (!xdr_u_long(xdrs, &(rr->rj_vers.low)))
			return (FALSE);
		return (xdr_u_long(xdrs, &(rr->rj_vers.high)));

	case AUTH_ERROR:
		return (xdr_enum(xdrs, (enum_t *)&(rr->rj_why)));
		
	default:
		fprintf(stderr,"%s default:\n",__FUNCTION__);
		break;
	}
	return (FALSE);
}

static struct xdr_discrim reply_dscrm[3] = {
	{ (int)MSG_ACCEPTED, xdr_accepted_reply },
	{ (int)MSG_DENIED, xdr_rejected_reply },
	{ __dontcare__, NULL_xdrproc_t } };

/*
 * XDR a reply message
 */
MINI_XDR_EXPORT
bool_t
xdr_replymsg(xdrs, rmsg)
	register XDR *xdrs;
	register struct rpc_msg *rmsg;
{
	XDR_RPC_DEBUG("  ");
	if (xdr_u_long(xdrs, &(rmsg->rm_xid)) && xdr_enum(xdrs, (enum_t *)&(rmsg->rm_direction)) && (rmsg->rm_direction == REPLY) ){
		bool_t bRet;
		XDR_RPC_DEBUG("  ");
		bRet = (xdr_union(xdrs, (enum_t *)&(rmsg->rm_reply.rp_stat),(caddr_t)&(rmsg->rm_reply.ru), reply_dscrm, NULL_xdrproc_t));
		XDR_RPC_DEBUG("  ");
		return bRet;
	}
	return (FALSE);
}


/*
 * Serializes the "static part" of a call message header.
 * The fields include: rm_xid, rm_direction, rpcvers, prog, and vers.
 * The rm_xid is not really static, but the user can easily munge on the fly.
 */
MINI_XDR_EXPORT
bool_t
xdr_callhdr(xdrs, cmsg)
	register XDR *xdrs;
	register struct rpc_msg *cmsg;
{

	cmsg->rm_direction = CALL;
	cmsg->rm_call.cb_rpcvers = RPC_MSG_VERSION;
	if (
	    (xdrs->x_op == XDR_ENCODE) &&
		xdr_u_long(xdrs, &(cmsg->rm_xid)) &&
	    xdr_enum(xdrs, (enum_t *)&(cmsg->rm_direction)) &&
		xdr_u_long(xdrs, &(cmsg->rm_call.cb_rpcvers)) &&
		xdr_u_long(xdrs, &(cmsg->rm_call.cb_prog)) )
	    return (xdr_u_long(xdrs, &(cmsg->rm_call.cb_vers)));
	return (FALSE);
}

/* ************************** Client utility routine ************* */

static void
accepted(acpt_stat, error)
	register enum accept_stat acpt_stat;
	register struct rpc_err *error;
{

	switch (acpt_stat) {

	case PROG_UNAVAIL:
		error->re_status = RPC_PROGUNAVAIL;
		return;

	case PROG_MISMATCH:
		error->re_status = RPC_PROGVERSMISMATCH;
		return;

	case PROC_UNAVAIL:
		error->re_status = RPC_PROCUNAVAIL;
		return;

	case GARBAGE_ARGS:
		error->re_status = RPC_CANTDECODEARGS;
		return;

	case SYSTEM_ERR:
		error->re_status = RPC_SYSTEMERROR;
		return;

	case SUCCESS:
		error->re_status = RPC_SUCCESS;
		return;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_ACCEPTED;
	error->re_lb.s2 = (long)acpt_stat;
}

static void 
rejected(rjct_stat, error)
	register enum reject_stat rjct_stat;
	register struct rpc_err *error;
{

	switch (rjct_stat) {

	case RPC_VERSMISMATCH_REJ_STAT:
		error->re_status = RPC_VERSMISMATCH;
		return;

	case AUTH_ERROR:
		error->re_status = RPC_AUTHERROR;
		return;
	default:
		fprintf(stderr,"%s default:\n",__FUNCTION__);
		break;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_DENIED;
	error->re_lb.s2 = (long)rjct_stat;
}

/*
 * given a reply message, fills in the error
 */
void
_seterr_reply(msg, error)
	struct rpc_msg *msg;
	struct rpc_err *error;
{

	/* optimized for normal, SUCCESSful case */
	switch (msg->rm_reply.rp_stat) {

	case MSG_ACCEPTED:
		if (msg->acpted_rply.ar_stat == SUCCESS) {
			error->re_status = RPC_SUCCESS;
			return;
		};
		accepted(msg->acpted_rply.ar_stat, error);
		break;

	case MSG_DENIED:
		rejected(msg->rjcted_rply.rj_stat, error);
		break;

	default:
		error->re_status = RPC_FAILED;
		error->re_lb.s1 = (long)(msg->rm_reply.rp_stat);
		break;
	}
	switch (error->re_status) {

	case RPC_VERSMISMATCH:
		error->re_vers.low = msg->rjcted_rply.rj_vers.low;
		error->re_vers.high = msg->rjcted_rply.rj_vers.high;
		break;

	case RPC_AUTHERROR:
		error->re_why = msg->rjcted_rply.rj_why;
		break;

	case RPC_PROGVERSMISMATCH:
		error->re_vers.low = msg->acpted_rply.ar_vers.low;
		error->re_vers.high = msg->acpted_rply.ar_vers.high;
		break;
		
	default:
		fprintf(stderr,"%s default:\n",__FUNCTION__);
		break;
	}
}


/*///////////////// hidden globals /////////////////////*/
MINI_XDR_DLL_PRIVATE struct opaque_auth _null_auth;

MINI_XDR_END_C_DECLS
