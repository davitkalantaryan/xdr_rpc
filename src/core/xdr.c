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

/* @(#)xdr.c	2.1 88/07/29 4.0 RPCSRC */
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
static char sccsid[] = "@(#)xdr.c 1.35 87/08/12";
#endif

/*
 * xdr.c, Generic XDR routines implementation.
 *
 * Copyright (C) 1986, Sun Microsystems, Inc.
 *
 * These are the "generic" xdr routines used to serialize and de-serialize
 * most common data items.  See xdr.h for more info on the interface to
 * xdr.
 */

#include <rpc/wrpc_first_com_include.h>
#include "xdr_rpc_debug.h"
#include <rpc/wrpc_first_com_include.h>
#include <stdio.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include "mini_xdr_rpc_src_private.h"


/*
 * constants specific to the xdr "protocol"
 */
#define XDR_FALSE			((long) 0)
#define XDR_TRUE			((long) 1)
#define LASTUNSIGNED		((u_int) 0-1)

MINI_XDR_BEGIN_C_DECLS

/*
 * for unit alignment
 */
static char xdr_zero[BYTES_PER_XDR_UNIT] = { 0, 0, 0, 0 };


/*
 * Free a data structure using XDR
 * Not a filter, but a convenient utility nonetheless
 */
MINI_XDR_EXPORT
void
xdr_free(proc, objp)
	xdrproc_t proc;
	char *objp;
{
	XDR x;

	x.x_op = XDR_FREE;
	(*proc)(&x, objp);
}


/*
 * XDR nothing
 */
MINI_XDR_EXPORT
bool_t
xdr_void(XDR *xdrs,void* p, ...)
	/* XDR *xdrs; */
	/* caddr_t addr; */
{
	XDR_RPC_UNUSED(xdrs);
	XDR_RPC_UNUSED(p);
	return (TRUE);
}


/*
 * XDR integers
 */
MINI_XDR_EXPORT
bool_t
xdr_int(XDR * xdrs, void* ipp, ...)
{
	int *ip = (int *)ipp;
#ifdef lint
	(void) (xdr_short(xdrs, (short *)ip));
	return (xdr_long(xdrs, (long *)ip));
#else

#ifdef _WIN64
	return (xdr_short(xdrs, ip));
#else

	if (sizeof (int) == sizeof (long)) {
		return (xdr_long(xdrs,ip));
	} else {
		return (xdr_short(xdrs,ip));
	}
#endif
#endif
}


/*
 * XDR unsigned integers
 */
MINI_XDR_EXPORT
bool_t
xdr_u_int(XDR * xdrs, void* upp, ...)
{
	u_int *up = (u_int *)upp;
#ifdef lint
	(void) (xdr_short(xdrs, (short *)up));
	return (xdr_u_long(xdrs, (u_long *)up));
#else
	if (sizeof (u_int) == sizeof (u_long)) {
		return (xdr_u_long(xdrs, up));
	} else {
		return (xdr_short(xdrs, up));
	}
#endif
}


/*
 * XDR long integers
 * same as xdr_u_long - open coded to save a proc call!
 */
MINI_XDR_EXPORT
bool_t
xdr_long(XDR * xdrs, void* lpp, ...)
{
	long *lp = (long *)lpp;

	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONG(xdrs, lp));

	if (xdrs->x_op == XDR_DECODE)
		return XDR_GETLONG(xdrs, lp);

	if (xdrs->x_op == XDR_FREE)
		return (TRUE);

	return (FALSE);
}


/*
 * XDR long integers
 * same as xdr_u_long - open coded to save a proc call!
 */
MINI_XDR_EXPORT
bool_t
xdr_longlong(XDR* xdrs, void* lpp, ...)
{
	long long* lp = (long long*)lpp;

	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONGLONG(xdrs, lp));

	if (xdrs->x_op == XDR_DECODE)
		return XDR_GETLONGLONG(xdrs, lp);

	if (xdrs->x_op == XDR_FREE)
		return (TRUE);

	return (FALSE);
}


MINI_XDR_EXPORT
bool_t
xdr_long_as_ptrdiff_t(XDR * xdrs, void* lpp, ...)
{
	ptrdiff_t *lp = (ptrdiff_t *)lpp;
	int bRet = FALSE;
	long longVal = lp?(long)*lp:0;
	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONG(xdrs, &longVal));

	if (xdrs->x_op == XDR_DECODE){
		bRet = XDR_GETLONG(xdrs, &longVal);
	}

	if (xdrs->x_op == XDR_FREE)
		return (TRUE);

	if(lp){*lp=longVal;}

	return bRet;
}


/*
 * XDR unsigned long integers
 * same as xdr_long - open coded to save a proc call!
 */
MINI_XDR_EXPORT
bool_t
xdr_u_long(XDR * xdrs, void* ulpp, ...)
{
	u_long *ulp = (u_long *)ulpp;
	if (xdrs->x_op == XDR_DECODE)
		return (XDR_GETLONG(xdrs, (long*)ulp));
	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONG(xdrs, (long*)ulp));
	if (xdrs->x_op == XDR_FREE)
		return (TRUE);
	return (FALSE);
}


MINI_XDR_EXPORT
bool_t
xdr_u_longlong (XDR* xdrs, void* ulpp, ...)
{
	unsigned long long* ulp = (unsigned long long*)ulpp;
	if (xdrs->x_op == XDR_DECODE)
		return (XDR_GETLONGLONG(xdrs, (long long*)ulp));
	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONGLONG(xdrs, (long long*)ulp));
	if (xdrs->x_op == XDR_FREE)
		return (TRUE);
	return (FALSE);
}


/*
 * XDR unsigned long integers
 * same as xdr_long - open coded to save a proc call!
 */
MINI_XDR_EXPORT
bool_t
xdr_u_long_as_size_t(XDR * xdrs, void* ulpp, ...)
{
	size_t *ulp = (size_t *)ulpp;
	int bRet = FALSE;
	long longVal = ulp?(long)*ulp:0;
	if (xdrs->x_op == XDR_DECODE)
		bRet = (XDR_GETLONG(xdrs, &longVal));
	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONG(xdrs, &longVal));
	if (xdrs->x_op == XDR_FREE)
		return (TRUE);
	if (ulp) { *ulp = (size_t)longVal; }
	return bRet;
}


MINI_XDR_EXPORT
bool_t
xdr_u_long_as_unsigned_long(XDR * xdrs, void* ulpp, ...)
{
	unsigned long *ulp = (unsigned long *)ulpp;
	int bRet = FALSE;
	long longVal = ulp?(long)*ulp:0;
	if (xdrs->x_op == XDR_DECODE)
		bRet = (XDR_GETLONG(xdrs, &longVal));
	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONG(xdrs, &longVal));
	if (xdrs->x_op == XDR_FREE)
		return (TRUE);
	if (ulp) { *ulp = (unsigned long)longVal; }
	return bRet;
}


/*
 * XDR short integers
 */
MINI_XDR_EXPORT
bool_t
xdr_short(XDR_RPC_REGISTER XDR * xdrs, void* spp, ...)
{
	short *sp = (short *)spp;
	long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (long) *sp;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {
			return (FALSE);
		}
		*sp = (short) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}


/*
 * XDR unsigned short integers
 */
MINI_XDR_EXPORT
bool_t
xdr_u_short(XDR_RPC_REGISTER XDR * xdrs, void* uspp, ...)
{
	u_short *usp = (u_short *)uspp;
	long l;
	
	XDR_RPC_DEBUG("  ");

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (long) *usp;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {
			return (FALSE);
		}
		*usp = (u_short) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}


/*
 * XDR a char
 */
MINI_XDR_EXPORT
bool_t
xdr_char(XDR * xdrs, void* cpp, ...)
{
	char *cp = (char *)cpp;
	int i;

	i = (*cp);
	if (!xdr_int(xdrs, &i)) {
		return (FALSE);
	}
	*cp = (char)i;
	return (TRUE);
}


/*
 * XDR an unsigned char
 */
MINI_XDR_EXPORT
bool_t
xdr_u_char(XDR * xdrs, void* ucpp, ...)
{
	u_char *ucp = (u_char *)ucpp;
	u_int u;

	u = (*ucp);
	if (!xdr_u_int(xdrs, &u)) {
		return (FALSE);
	}
	*ucp = (u_char)u;
	return (TRUE);
}


/*
 * XDR booleans
 */
MINI_XDR_EXPORT
bool_t
xdr_bool(XDR_RPC_REGISTER XDR * xdrs, void* bpp, ...)
{
	bool_t *bp = (bool_t *)bpp;
	long lb;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		lb = *bp ? XDR_TRUE : XDR_FALSE;
		return (XDR_PUTLONG(xdrs, &lb));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &lb)) {
			return (FALSE);
		}
		*bp = (lb == XDR_FALSE) ? FALSE : TRUE;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}


/*
 * XDR enumerations
 */
MINI_XDR_EXPORT
bool_t
xdr_enum(XDR * xdrs, void* epp, ...)
{
	enum_t *ep = (enum_t *)epp;
#ifndef lint
	enum sizecheck { SIZEVAL };	/* used to find the size of an enum */

	/*
	 * enums are treated as ints
	 */
	if (sizeof (enum sizecheck) == sizeof (long)) {
		return (xdr_long(xdrs, (long *)ep));
	} else if (sizeof (enum sizecheck) == sizeof (short)) {
		return (xdr_short(xdrs, (short *)ep));
	} else {
		return (FALSE);
	}
#else
	(void) (xdr_short(xdrs, (short *)ep));
	return (xdr_long(xdrs, (long *)ep));
#endif
}


/*
 * XDR opaque data
 * Allows the specification of a fixed size sequence of opaque bytes.
 * cp points to the opaque object and cnt gives the byte length.
 */
MINI_XDR_EXPORT
bool_t
xdr_opaque(XDR_RPC_REGISTER XDR * xdrs, void* cpp, ...)
{
	caddr_t cp = (caddr_t )cpp;
	va_list ap;
	register u_int cnt;
	register u_int rndup;
	static char crud[BYTES_PER_XDR_UNIT];
	
	va_start(ap, cpp);
	cnt = va_arg(ap,u_int);
	va_end(ap);

	/*
	 * if no data we are done
	 */
	if (cnt == 0)
		return (TRUE);

	/*
	 * round byte count to full xdr units
	 */
	rndup = cnt % BYTES_PER_XDR_UNIT;
	if (rndup > 0)
		rndup = BYTES_PER_XDR_UNIT - rndup;

	if (xdrs->x_op == XDR_DECODE) {
		if (!XDR_GETBYTES(xdrs, cp, cnt)) {
			return (FALSE);
		}
		if (rndup == 0)
			return (TRUE);
		return (XDR_GETBYTES(xdrs, (caddr_t)crud, rndup));
	}

	if (xdrs->x_op == XDR_ENCODE) {
		if (!XDR_PUTBYTES(xdrs, cp, cnt)) {
			return (FALSE);
		}
		if (rndup == 0)
			return (TRUE);
		return (XDR_PUTBYTES(xdrs, xdr_zero, rndup));
	}

	if (xdrs->x_op == XDR_FREE) {
		return (TRUE);
	}

	return (FALSE);
}


/*
 * XDR counted bytes
 * *cpp is a pointer to the bytes, *sizep is the count.
 * If *cpp is NULL maxsize bytes are allocated
 */
MINI_XDR_EXPORT
bool_t
xdr_bytes(XDR_RPC_REGISTER XDR * xdrs, void* cppp, ...)
{
	char** cpp = (char** )cppp;
	va_list ap;
	register u_int *sizep;
	u_int maxsize;
	register char *sp = *cpp;  /* sp is the actual string pointer */
	register u_int nodesize;
	
	va_start(ap, cppp);
	sizep = va_arg(ap,u_int*);
	maxsize = va_arg(ap,u_int);
	va_end(ap);

	/*
	 * first deal with the length since xdr bytes are counted
	 */
	if (! xdr_u_int(xdrs, sizep)) {
		return (FALSE);
	}
	nodesize = *sizep;
	if ((nodesize > maxsize) && (xdrs->x_op != XDR_FREE)) {
		return (FALSE);
	}

	/*
	 * now deal with the actual bytes
	 */
	switch (xdrs->x_op) {

	case XDR_DECODE:
		if (nodesize == 0) {
			return (TRUE);
		}
		if (sp == NULL) {
			*cpp = sp = (char *)mem_alloc(nodesize);
		}
		if (!sp) {
			(void) fprintf(stderr, "xdr_bytes: out of memory\n");
			return (FALSE);
		}
		/* fall into ... */

	case XDR_ENCODE:
		return (xdr_opaque(xdrs, sp, nodesize));

	case XDR_FREE:
		if (sp != NULL) {
			mem_free(sp, nodesize);
			*cpp = NULL;
		}
		return (TRUE);
	}
	return (FALSE);
}


/*
 * Implemented here due to commonality of the object.
 */
MINI_XDR_EXPORT
bool_t
xdr_netobj(XDR * xdrs, void* npp, ...)
{
	struct netobj *np = (struct netobj *)npp;
	return (xdr_bytes(xdrs, &np->n_bytes, &np->n_len, MAX_NETOBJ_SZ));
}


/*
 * XDR a descriminated union
 * Support routine for discriminated unions.
 * You create an array of xdrdiscrim structures, terminated with
 * an entry with a null procedure pointer.  The routine gets
 * the discriminant value and then searches the array of xdrdiscrims
 * looking for that value.  It calls the procedure given in the xdrdiscrim
 * to handle the discriminant.  If there is no specific routine a default
 * routine may be called.
 * If there is no specific or default routine an error is returned.
 */
MINI_XDR_EXPORT
bool_t
xdr_union(XDR_RPC_REGISTER XDR * xdrs, void* dscmpp, ...)
{
	enum_t* dscmp = (enum_t* )dscmpp;
	va_list ap;
	
	char *unp;		/* the union itself */
	struct xdr_discrim *choices;	/* [value, xdr proc] for each arm */
	xdrproc_t dfault;	/* default xdr routine */
	
	register enum_t dscm;
	
	va_start(ap, dscmpp);
	unp = va_arg(ap,char*);
	choices = va_arg(ap,struct xdr_discrim *);
	dfault = va_arg(ap,xdrproc_t);
	va_end(ap);

	/*
	 * we deal with the discriminator;  it's an enum
	 */
	if (! xdr_enum(xdrs, dscmp)) {
		return (FALSE);
	}
	dscm = *dscmp;

	/*
	 * search choices for a value that matches the discriminator.
	 * if we find one, execute the xdr routine for that value.
	 */
	for (; choices->proc != NULL_xdrproc_t; choices++) {
		if (choices->value == dscm)
			return ((*(choices->proc))(xdrs, unp, LASTUNSIGNED));
	}

	/*
	 * no match - execute the default xdr routine if there is one
	 */
	return ((dfault == NULL_xdrproc_t) ? FALSE :
	    (*dfault)(xdrs, unp, LASTUNSIGNED));
}


/*
 * Non-portable xdr primitives.
 * Care should be taken when moving these routines to new architectures.
 */


/*
 * XDR null terminated ASCII strings
 * xdr_string deals with "C strings" - arrays of bytes that are
 * terminated by a NULL character.  The parameter cpp references a
 * pointer to storage; If the pointer is null, then the necessary
 * storage is allocated.  The last parameter is the max allowed length
 * of the string as specified by a protocol.
 */
MINI_XDR_EXPORT
bool_t
xdr_string(XDR_RPC_REGISTER XDR * xdrs, void* cppp, ...)
{
	char** cpp = (char** )cppp;
	va_list ap;
	u_int maxsize;
	
	register char *sp = *cpp;  /* sp is the actual string pointer */
	u_int size;
	u_int nodesize;
	
	va_start(ap, cppp);
	maxsize = va_arg(ap,u_int);
	va_end(ap);

	/*
	 * first deal with the length since xdr strings are counted-strings
	 */
	switch (xdrs->x_op) {
	case XDR_FREE:
		if (sp == NULL) {
			return(TRUE);	/* already free */
		}
		/* fall through... */
	case XDR_ENCODE:
		size = (u_int)strlen(sp);
		break;
		
	default:
		fprintf(stderr,"%s default:\n",__FUNCTION__);
		break;
	}
	if (! xdr_u_int(xdrs, &size)) {
		return (FALSE);
	}
	if (size > maxsize) {
		return (FALSE);
	}
	nodesize = size + 1;

	/*
	 * now deal with the actual bytes
	 */
	switch (xdrs->x_op) {

	case XDR_DECODE:
		if (nodesize == 0) {
			return (TRUE);
		}
		if (sp == NULL)
			*cpp = sp = (char *)mem_alloc(nodesize);
		if (sp == NULL) {
			(void) fprintf(stderr, "xdr_string: out of memory\n");
			return (FALSE);
		}
		sp[size] = 0;
		/* fall into ... */

	case XDR_ENCODE:
		return (xdr_opaque(xdrs, sp, size));

	case XDR_FREE:
		mem_free(sp, nodesize);
		*cpp = NULL;
		return (TRUE);
	}
	return (FALSE);
}

/*
 * Wrapper for xdr_string that can be called directly from
 * routines like clnt_call
 */
MINI_XDR_EXPORT
bool_t
xdr_wrapstring(XDR * xdrs, void* cppp, ...)
{
	char** cpp = (char** )cppp;
	if (xdr_string(xdrs, cpp, LASTUNSIGNED)) {
		return (TRUE);
	}
	return (FALSE);
}


MINI_XDR_END_C_DECLS
