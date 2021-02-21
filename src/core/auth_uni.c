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

/* @(#)auth_unix.c	2.2 88/08/01 4.0 RPCSRC */
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
static char sccsid[] = "@(#)auth_unix.c 1.19 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * auth_unix.c, Implements UNIX style authentication parameters.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * The system is very weak.  The client uses no encryption for it's
 * credentials and only sends null verifiers.  The server sends backs
 * null verifiers or optionally a verifier that suggests a new short hand
 * for the credentials.
 *
 */

#include "rpc/xdr.h"
#include "rpc/auth.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <stdio.h>
#include <rpc/auth_unix.h>
#include <rpc/doocs_rpc_unix_like_functions.h>
#include "mini_xdr_rpc_src_private.h"

MINI_XDR_BEGIN_C_DECLS


static bool_t marshal_new_auth(register AUTH* auth);

/*
 * Unix authenticator operations vector
 */
static void		authunix_nextverf(struct AUTH_struct *auth);
static bool_t	authunix_marshal(struct AUTH_struct *, XDR *);
static bool_t	authunix_validate(register struct AUTH_struct *auth,struct opaque_auth* verf);
static bool_t	authunix_refresh(register struct AUTH_struct *auth);
static void		authunix_destroy(register struct AUTH_struct *auth);

static struct auth_ops auth_unix_ops = {
	authunix_nextverf,
	authunix_marshal,
	authunix_validate,
	authunix_refresh,
	authunix_destroy
};

/*
 * This struct is pointed to by the ah_private field of an auth_handle.
 */
struct audata {
	struct opaque_auth	au_origcred;	/* original credentials */
	struct opaque_auth	au_shcred;	/* short hand cred */
	u_long				au_shfaults;	/* short hand cache faults */
	char				au_marshed[MAX_AUTH_BYTES];
	u_int				au_mpos;	/* xdr pos at end of marshed */
};
#define	AUTH_PRIVATE(auth)	((struct audata *)auth->ah_private)



/*
 * Create a unix style authenticator.
 * Returns an auth handle with the given stuff in it.
 */
// MINI_XDR_EXPORT AUTH * authunix_create(char* machname, uid_t uid, gid_t gid, int len, gid_t * aup_gids);
MINI_XDR_EXPORT
AUTH *
authunix_create(machname, uid, gid, len, aup_gids)
	char *machname;
	uid_t uid;
	gid_t gid;
	int len;
	gid_t *aup_gids;
{
	struct authunix_parms aup;
	char mymem[MAX_AUTH_BYTES];
	struct timeval now;
	XDR xdrs;
	register AUTH *auth;
	register struct audata *au;

	/*
	 * Allocate and set up auth handle
	 */
	auth = (AUTH *)mem_alloc(sizeof(*auth));
#ifndef KERNEL
	if (!auth) {
		(void)fprintf(stderr, "authunix_create: out of memory\n");
		return (NULL);
	}
#endif
	au = (struct audata *)mem_alloc(sizeof(*au));
#ifndef KERNEL
	if (!au) {
		(void)fprintf(stderr, "authunix_create: out of memory\n");
		return (NULL);
	}
#endif
	auth->ah_ops = &auth_unix_ops;
	auth->ah_private = (caddr_t)au;
	auth->ah_verf = au->au_shcred = _null_auth;
	au->au_shfaults = 0;

	/*
	 * fill in param struct from the given params
	 */
	(void)gettimeofday(&now,  (struct timezone *)0);
	aup.aup_time = now.tv_sec;
	aup.aup_machname = machname;
	aup.aup_uid = uid;
	aup.aup_gid = gid;
	aup.aup_len = (u_int)len;
	aup.aup_gids = aup_gids;

	/*
	 * Serialize the parameters into origcred
	 */
	xdrmem_create(&xdrs, mymem, MAX_AUTH_BYTES, XDR_ENCODE);
	if (! xdr_authunix_parms(&xdrs, &aup))
		abort();
	au->au_origcred.oa_length = len = XDR_GETPOS(&xdrs);
	au->au_origcred.oa_flavor = AUTH_UNIX;
#ifdef KERNEL
	au->au_origcred.oa_base = mem_alloc((u_int) len);
#else
	if ((au->au_origcred.oa_base = mem_alloc((u_int) len)) == NULL) {
		(void)fprintf(stderr, "authunix_create: out of memory\n");
		return (NULL);
	}
#endif
	bcopy(mymem, au->au_origcred.oa_base, (u_int)len);

	/*
	 * set auth handle to reflect new cred.
	 */
	auth->ah_cred = au->au_origcred;
	marshal_new_auth(auth);
	return (auth);
}

/*
 * Returns an auth handle with parameters determined by doing lots of
 * syscalls.
 */
#if 1
// MINI_XDR_EXPORT_FNL
MINI_XDR_EXPORT
AUTH *
authunix_create_default(void)
{
	register int len;
	char machname[MAX_MACHINE_NAME + 1];
	register int uid;
	register int gid;
	int gids[NGRPS];

	if (gethostname(machname, MAX_MACHINE_NAME) == -1)
		abort();
	machname[MAX_MACHINE_NAME] = 0;
#ifdef _WIN32
/* who knows anything better? me!!!*/
	//uid = geteuid();
	//gid = getegid();
	uid = 0;
	gid = 0;
	len = 1;
	gids[0] = 0;
#else
	uid = geteuid();
	gid = getegid();
	if ((len = getgroups(NGRPS, gids)) < 0)
		abort();
#endif
	return (authunix_create(machname, uid, gid, len, gids));
}
#endif

MINI_XDR_EXPORT
bool_t
xdr_authunix_parms(XDR_RPC_REGISTER XDR * xdrs, void* pp, ...)
{
	register struct authunix_parms* p = (struct authunix_parms*)pp;

	if (xdr_u_long(xdrs, &(p->aup_time))
		&& xdr_string(xdrs, &(p->aup_machname), MAX_MACHINE_NAME)
		&& xdr_u_int(xdrs, &(p->aup_uid))
		&& xdr_u_int(xdrs, &(p->aup_gid))
		&& xdr_array(xdrs, (caddr_t*) & (p->aup_gids),
			&(p->aup_len), NGRPS, sizeof(int), &xdr_int)) {
		return (TRUE);
	}
	return (FALSE);
}

/*
 * authunix operations
 */

static void
authunix_nextverf(auth)
	struct AUTH_struct *auth;
{
	XDR_RPC_UNUSED(auth);
	/* no action necessary */
}

static bool_t
authunix_marshal(auth, xdrs)
	struct AUTH_struct *auth;
	XDR *xdrs;
{
	register struct audata *au = AUTH_PRIVATE(auth);

	return (XDR_PUTBYTES(xdrs, au->au_marshed, au->au_mpos));
}

static bool_t
authunix_validate(auth, verf)
	register struct AUTH_struct *auth;
	struct opaque_auth* verf;
{
	register struct audata *au;
	XDR xdrs;

	if (verf->oa_flavor == AUTH_SHORT) {
		au = AUTH_PRIVATE(auth);
		xdrmem_create(&xdrs, verf->oa_base, verf->oa_length, XDR_DECODE);

		if (au->au_shcred.oa_base != NULL) {
			mem_free(au->au_shcred.oa_base,
			    au->au_shcred.oa_length);
			au->au_shcred.oa_base = NULL;
		}
		if (xdr_opaque_auth(&xdrs, &au->au_shcred)) {
			auth->ah_cred = au->au_shcred;
		} else {
			xdrs.x_op = XDR_FREE;
			(void)xdr_opaque_auth(&xdrs, &au->au_shcred);
			au->au_shcred.oa_base = NULL;
			auth->ah_cred = au->au_origcred;
		}
		marshal_new_auth(auth);
	}
	return (TRUE);
}

static bool_t
authunix_refresh(auth)
	register struct AUTH_struct *auth;
{
	register struct audata *au = AUTH_PRIVATE(auth);
	struct authunix_parms aup;
	struct timeval now;
	XDR xdrs;
	register int stat;

	if (auth->ah_cred.oa_base == au->au_origcred.oa_base) {
		/* there is no hope.  Punt */
		return (FALSE);
	}
	au->au_shfaults ++;

	/* first deserialize the creds back into a struct authunix_parms */
	aup.aup_machname = NULL;
	aup.aup_gids = NULL;
	xdrmem_create(&xdrs, au->au_origcred.oa_base,
	    au->au_origcred.oa_length, XDR_DECODE);
	stat = xdr_authunix_parms(&xdrs, &aup);
	if (! stat)
		goto done;

	/* update the time and serialize in place */
	(void)gettimeofday(&now, (struct timezone *)0);
	aup.aup_time = now.tv_sec;
	xdrs.x_op = XDR_ENCODE;
	XDR_SETPOS(&xdrs, 0);
	stat = xdr_authunix_parms(&xdrs, &aup);
	if (! stat)
		goto done;
	auth->ah_cred = au->au_origcred;
	marshal_new_auth(auth);
done:
	/* free the struct authunix_parms created by deserializing */
	xdrs.x_op = XDR_FREE;
	(void)xdr_authunix_parms(&xdrs, &aup);
	XDR_DESTROY(&xdrs);
	return (stat);
}

static void
authunix_destroy(auth)
	register struct AUTH_struct *auth;
{
	register struct audata *au = AUTH_PRIVATE(auth);

	mem_free(au->au_origcred.oa_base, au->au_origcred.oa_length);

	if (au->au_shcred.oa_base != NULL)
		mem_free(au->au_shcred.oa_base, au->au_shcred.oa_length);

	mem_free(auth->ah_private, sizeof(struct audata));

	if (auth->ah_verf.oa_base != NULL)
		mem_free(auth->ah_verf.oa_base, auth->ah_verf.oa_length);

	//mem_free((caddr_t)auth, sizeof(*auth));
	mem_free(auth, sizeof(*auth));
}

/*
 * Marshals (pre-serializes) an auth struct.
 * sets private data, au_marshed and au_mpos
 */
static bool_t
marshal_new_auth(auth)
	register AUTH *auth;
{
	XDR		xdr_stream;
	register XDR	*xdrs = &xdr_stream;
	register struct audata *au = AUTH_PRIVATE(auth);

	xdrmem_create(xdrs, au->au_marshed, MAX_AUTH_BYTES, XDR_ENCODE);
	if ((! xdr_opaque_auth(xdrs, &(auth->ah_cred))) ||
	    (! xdr_opaque_auth(xdrs, &(auth->ah_verf)))) {
		perror("auth_none.c - Fatal marshalling problem");
	} else {
		au->au_mpos = XDR_GETPOS(xdrs);
	}
	XDR_DESTROY(xdrs);
	return 1;
}


MINI_XDR_END_C_DECLS
