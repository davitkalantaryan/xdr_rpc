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

/* @(#)xdr_rec.c	2.2 88/08/01 4.0 RPCSRC */
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
static char sccsid[] = "@(#)xdr_rec.c 1.21 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * xdr_rec.c, Implements TCP/IP based XDR streams with a "record marking"
 * layer above tcp (for rpc's use).
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These routines interface XDRSTREAMS to a tcp/ip connection.
 * There is a record marking layer between the xdr stream
 * and the tcp transport level.  A record is composed on one or more
 * record fragments.  A record fragment is a thirty-two bit header followed
 * by n bytes of data, where n is contained in the header.  The header
 * is represented as a htonl(u_long).  Thegh order bit encodes
 * whether or not the fragment is the last fragment of the record
 * (1 => fragment is last, 0 => more fragments to follow.
 * The other 31 bits encode the byte length of the fragment.
 */

#include <rpc/wrpc_first_com_include.h>
#include "xdr_rpc_debug.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <stdio.h>
#include <stddef.h>
#ifdef _WIN32
#include <io.h>
#define lseek	_lseek
#else
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include "mini_xdr_rpc_src_private.h"
#include <stdint.h>

MINI_XDR_BEGIN_C_DECLS

static u_int	fix_buf_size(register u_int s);

static bool_t	xdrrec_getlong(XDR *xdrs,register long *lp);
static bool_t	xdrrec_putlong(XDR *xdrs,long *lp);
static bool_t	xdrrec_getlonglong(XDR* xdrs, register long long* lp);
static bool_t	xdrrec_putlonglong(XDR* xdrs, long long* lp);
static bool_t	xdrrec_getbytes(XDR *xdrs,caddr_t addr,u_int len);
static bool_t	xdrrec_putbytes(XDR *xdrs,caddr_t addr,u_int len);
static u_int	xdrrec_getpos(XDR *xdrs);
static bool_t	xdrrec_setpos(XDR *xdrs,u_int pos);
static long *	xdrrec_inline(XDR *xdrs, u_int len);
static void	xdrrec_destroy(register XDR *xdrs);


static struct  xdr_ops xdrrec_ops = {
	xdrrec_getlong,
	xdrrec_putlong,
	xdrrec_getlonglong,
	xdrrec_putlonglong,
	xdrrec_getbytes,
	xdrrec_putbytes,
	xdrrec_getpos,
	xdrrec_setpos,
	xdrrec_inline,
	xdrrec_destroy
};

/*
 * A record is composed of one or more record fragments.
 * A record fragment is a two-byte header followed by zero to
 * 2**32-1 bytes.  The header is treated as a long unsigned and is
 * encode/decoded to the network via htonl/ntohl.  The low order 31 bits
 * are a byte count of the fragment.  The highest order bit is a boolean:
 * 1 => this fragment is the last fragment of the record,
 * 0 => this fragment is followed by more fragment(s).
 *
 * The fragment/record machinery is not general;  it is constructed to
 * meet the needs of xdr and rpc based on tcp.
 */

#define LAST_FRAG ((u_long)(1 << 31))

typedef struct rec_strm {
	caddr_t tcp_handle;
	caddr_t the_buffer;
	/*
	 * out-goung bits
	 */
	XdrRpcIo writeit;
	caddr_t out_base;	/* output buffer (points to frag header) */
	caddr_t out_finger;	/* next output position */
	caddr_t out_boundry;	/* data cannot up to this address */
	u_long *frag_header;	/* beginning of curren fragment */
	bool_t frag_sent;	/* true if buffer sent in middle of record */
	/*
	 * in-coming bits
	 */
	XdrRpcIo readit;
	u_long in_size;	/* fixed size of the input buffer */
	caddr_t in_base;
	caddr_t in_finger;	/* location of next byte to be had */
	caddr_t in_boundry;	/* can read up to this location */
	long fbtbc;		/* fragment bytes to be consumed */
	bool_t last_frag;
	u_int sendsize;
	u_int recvsize;
} RECSTREAM;

static bool_t flush_out(register RECSTREAM * rstrm, bool_t eor); /// Added by DK
static bool_t  set_input_fragment(register RECSTREAM * rstrm); /// Added by DK
static bool_t  get_input_bytes(register RECSTREAM *rstrm, register caddr_t addr, register int len); /// Added by DK
static bool_t  /* consumes input bytes; knows nothing about records! */
skip_input_bytes(register RECSTREAM *rstrm, long cnt); /// Added by DK

/*
 * Create an xdr handle for xdrrec
 * xdrrec_create fills in xdrs.  Sendsize and recvsize are
 * send and recv buffer sizes (0 => use default).
 * tcp_handle is an opaque handle that is passed as the first parameter to
 * the procedures readit and writeit.  Readit and writeit are read and
 * write respectively.   They are like the system
 * calls expect that they take an opaque handle rather than an fd.
 */
MINI_XDR_EXPORT
void
xdrrec_create(xdrs, sendsize, recvsize, tcp_handle, readit, writeit)
	register XDR *xdrs;
	register u_int sendsize;
	register u_int recvsize;
	caddr_t tcp_handle;
	XdrRpcIo readit;  /* like read, but pass it a tcp_handle, not sock */
	XdrRpcIo writeit;  /* like write, but pass it a tcp_handle, not sock */
{
	register RECSTREAM *rstrm = (RECSTREAM *)mem_alloc(sizeof(RECSTREAM));

	if (!rstrm) {
		(void)fprintf(stderr, "xdrrec_create: out of memory\n");
		/*
		 *  This is bad.  Should rework xdrrec_create to
		 *  return a handle, and in this case return NULL
		 */
		return;
	}
	/*
	 * adjust sizes and allocate buffer quad byte aligned
	 */
	rstrm->sendsize = sendsize = fix_buf_size(sendsize);
	rstrm->recvsize = recvsize = fix_buf_size(recvsize);
	rstrm->the_buffer = mem_alloc(sendsize + recvsize + BYTES_PER_XDR_UNIT);
	if (!rstrm->the_buffer) {
		(void)fprintf(stderr, "xdrrec_create: out of memory\n");
		return;
	}
	for (rstrm->out_base = rstrm->the_buffer;
		(size_t)rstrm->out_base % BYTES_PER_XDR_UNIT != 0;
		rstrm->out_base++);
	rstrm->in_base = rstrm->out_base + sendsize;
	/*
	 * now the rest ...
	 */
	xdrs->x_ops = &xdrrec_ops;
	xdrs->x_private = (caddr_t)rstrm;
	rstrm->tcp_handle = tcp_handle;
	rstrm->readit = readit;
	rstrm->writeit = writeit;
	rstrm->out_finger = rstrm->out_boundry = rstrm->out_base;
	rstrm->frag_header = (u_long *)rstrm->out_base;
	rstrm->out_finger += sizeof(u_long);
	rstrm->out_boundry += sendsize;
	rstrm->frag_sent = FALSE;
	rstrm->in_size = recvsize;
	rstrm->in_boundry = rstrm->in_base;
	rstrm->in_finger = (rstrm->in_boundry += recvsize);
	rstrm->fbtbc = 0;
	rstrm->last_frag = TRUE;
}


/*
 * The reoutines defined below are the xdr ops which will go into the
 * xdr handle filled in by xdrrec_create.
 */

static bool_t
xdrrec_getlong(xdrs, lp)
	XDR *xdrs;
	register long *lp;
{
	register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	register long *buflp = (long *)(rstrm->in_finger);
	long mylong;

	/* first try the inline, fast case */
	if ((rstrm->fbtbc >= sizeof(long)) && (rstrm->in_boundry - (caddr_t)buflp) >= sizeof(long) ) {
		*lp = (long)ntohl((u_long)(*buflp));
		rstrm->fbtbc -= sizeof(long);
		rstrm->in_finger += sizeof(long);
	} else {
		if (! xdrrec_getbytes(xdrs, (caddr_t)&mylong, sizeof(long)))
			return (FALSE);
		*lp = (long)ntohl((u_long)mylong);
	}
	return (TRUE);
}

static bool_t
xdrrec_putlong(xdrs, lp)
	XDR *xdrs;
	long *lp;
{
	register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	register long *dest_lp = ((long *)(rstrm->out_finger));

	if ((rstrm->out_finger += sizeof(long)) > rstrm->out_boundry) {
		/*
		 * this case should almost never happen so the code is
		 * inefficient
		 */
		rstrm->out_finger -= sizeof(long);
		rstrm->frag_sent = TRUE;
		if (! flush_out(rstrm, FALSE))
			return (FALSE);
		dest_lp = ((long *)(rstrm->out_finger));
		rstrm->out_finger += sizeof(long);
	}
	*dest_lp = (long)htonl((u_long)(*lp));
	return (TRUE);
}


// new api
static bool_t
xdrrec_getlonglong(XDR* xdrs, register long long* lp)
{
	register RECSTREAM* rstrm = (RECSTREAM*)(xdrs->x_private);
	register long long* buflp = (long long*)(rstrm->in_finger);
	long mylong;

	/* first try the inline, fast case */
	if ((rstrm->fbtbc >= sizeof(long long)) && (rstrm->in_boundry - (caddr_t)buflp) >= sizeof(long long)) {
		*lp = (long long)ntohll((unsigned long long)(*buflp));
		rstrm->fbtbc -= sizeof(long long);
		rstrm->in_finger += sizeof(long long);
	}
	else {
		if (!xdrrec_getbytes(xdrs, (caddr_t)&mylong, sizeof(long long)))
			return (FALSE);
		*lp = (long long)ntohll((unsigned long long)mylong);
	}
	return (TRUE);
}

static bool_t
xdrrec_putlonglong(XDR* xdrs, long long* lp)
{
	register RECSTREAM* rstrm = (RECSTREAM*)(xdrs->x_private);
	register long long* dest_lp = ((long long*)(rstrm->out_finger));

	if ((rstrm->out_finger += sizeof(long long)) > rstrm->out_boundry) {
		/*
		 * this case should almost never happen so the code is
		 * inefficient
		 */
		rstrm->out_finger -= sizeof(long long);
		rstrm->frag_sent = TRUE;
		if (!flush_out(rstrm, FALSE))
			return (FALSE);
		dest_lp = ((long long*)(rstrm->out_finger));
		rstrm->out_finger += sizeof(long long);
	}
	*dest_lp = (long long)htonll((unsigned long long)(*lp));
	return (TRUE);
}


static bool_t  /* must manage buffers, fragments, and records */
xdrrec_getbytes(xdrs, addr, len)
	XDR *xdrs;
	caddr_t addr;
	u_int len;
{
	register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	register int current;

	while (len > 0) {
		current = rstrm->fbtbc;
		if (current == 0) {
			if (rstrm->last_frag)
				return (FALSE);
			if (! set_input_fragment(rstrm))
				return (FALSE);
			continue;
		}
		current = (len < (u_int)current) ? len : current;
		if (! get_input_bytes(rstrm, addr, current))
			return (FALSE);
		addr += current;
		rstrm->fbtbc -= current;
		len -= current;
	}
	return (TRUE);
}

static bool_t
xdrrec_putbytes(xdrs, addr, len)
	XDR *xdrs;
	caddr_t addr;
	u_int len;
{
	register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	register u_int current;

	while (len > 0) {
		current = (u_int)(rstrm->out_boundry - rstrm->out_finger);
		current = (len < current) ? len : current;
		bcopy(addr, rstrm->out_finger, current);
		rstrm->out_finger += current;
		addr += current;
		len -= current;
		if (rstrm->out_finger == rstrm->out_boundry) {
			rstrm->frag_sent = TRUE;
			if (! flush_out(rstrm, FALSE))
				return (FALSE);
		}
	}
	return (TRUE);
}

static u_int
xdrrec_getpos(xdrs)
	XDR *xdrs;
{
	register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;
	register int64_t pos;

	pos = lseek((int)((ptrdiff_t)rstrm->tcp_handle), (long) 0, 1);
	if (pos != -1)
		switch (xdrs->x_op) {

		case XDR_ENCODE:
			pos += rstrm->out_finger - rstrm->out_base;
			break;

		case XDR_DECODE:
			pos -= rstrm->in_boundry - rstrm->in_finger;
			break;

		default:
			pos = (u_int) -1;
			break;
		}
	return ((u_int) pos);
}

static bool_t
xdrrec_setpos(xdrs, pos)
	XDR *xdrs;
	u_int pos;
{
	register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;
	u_int currpos = xdrrec_getpos(xdrs);
	int delta = currpos - pos;
	caddr_t newpos;

	if ((int)currpos != -1)
		switch (xdrs->x_op) {

		case XDR_ENCODE:
			newpos = rstrm->out_finger - delta;
			if ((newpos > (caddr_t)(rstrm->frag_header)) &&
				(newpos < rstrm->out_boundry)) {
				rstrm->out_finger = newpos;
				return (TRUE);
			}
			break;

		case XDR_DECODE:
			newpos = rstrm->in_finger - delta;
			if ((delta < (int)(rstrm->fbtbc)) &&
				(newpos <= rstrm->in_boundry) &&
				(newpos >= rstrm->in_base)) {
				rstrm->in_finger = newpos;
				rstrm->fbtbc -= delta;
				return (TRUE);
			}
			break;
			
		default:
			fprintf(stderr,"%s default:\n",__FUNCTION__);
			break;
		}
	return (FALSE);
}

static long *
xdrrec_inline(xdrs, len)
	XDR *xdrs;
	u_int len;
{
	register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;
	long * buf = NULL;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		if ((rstrm->out_finger + len) <= rstrm->out_boundry) {
			buf = (long *) rstrm->out_finger;
			rstrm->out_finger += len;
		}
		break;

	case XDR_DECODE:
		if ((((long)len) <= rstrm->fbtbc) &&
			((rstrm->in_finger + len) <= rstrm->in_boundry)) {
			buf = (long *) rstrm->in_finger;
			rstrm->fbtbc -= len;
			rstrm->in_finger += len;
		}
		break;
		
	default:
		fprintf(stderr,"%s default:\n",__FUNCTION__);
		break;
	}
	return (buf);
}

static void
xdrrec_destroy(xdrs)
	register XDR *xdrs;
{
	register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;

	mem_free(rstrm->the_buffer,
		rstrm->sendsize + rstrm->recvsize + BYTES_PER_XDR_UNIT);
	mem_free(rstrm, sizeof(RECSTREAM));
}


/*
 * Exported routines to manage xdr records
 */

/*
 * Before reading (deserializing from the stream, one should always call
 * this procedure to guarantee proper record alignment.
 */
bool_t
xdrrec_skiprecord(xdrs)
	XDR *xdrs;
{
	register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);

	while (rstrm->fbtbc > 0 || (! rstrm->last_frag)) {
		if (! skip_input_bytes(rstrm, rstrm->fbtbc))
			return (FALSE);
		rstrm->fbtbc = 0;
		if ((! rstrm->last_frag) && (! set_input_fragment(rstrm)))
			return (FALSE);
	}
	rstrm->last_frag = FALSE;
	return (TRUE);
}

/*
 * Look ahead fuction.
 * Returns TRUE iff there is no more input in the buffer
 * after consuming the rest of the current record.
 */
bool_t
xdrrec_eof(xdrs)
	XDR *xdrs;
{
	register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);

	while (rstrm->fbtbc > 0 || (! rstrm->last_frag)) {
		if (! skip_input_bytes(rstrm, rstrm->fbtbc))
			return (TRUE);
		rstrm->fbtbc = 0;
		if ((! rstrm->last_frag) && (! set_input_fragment(rstrm)))
			return (TRUE);
	}
	if (rstrm->in_finger == rstrm->in_boundry)
		return (TRUE);
	return (FALSE);
}


/*
 * The client must tell the package when an end-of-record has occurred.
 * The second paraemters tells whether the record should be flushed to the
 * (output) tcp stream.  (This let's the package support batched or
 * pipelined procedure calls.)  TRUE => immmediate flush to tcp connection.
 */
bool_t
xdrrec_endofrecord(xdrs, sendnow)
	XDR *xdrs;
	bool_t sendnow;
{
	register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	register u_long len;  /* fragment length */
	
	XDR_RPC_DEBUG("  ");

	if (sendnow || rstrm->frag_sent || (rstrm->out_finger + sizeof(u_long) >= rstrm->out_boundry)) {
		bool_t bRet;
		rstrm->frag_sent = FALSE;
		XDR_RPC_DEBUG("  ");
		bRet =  (flush_out(rstrm, TRUE));
		XDR_RPC_DEBUG("  ");
		return bRet;
	}
	len = POINTERS_DIFF(u_long, rstrm->out_finger,rstrm->frag_header) - sizeof(u_long);
	*(rstrm->frag_header) = htonl((unsigned long)len | LAST_FRAG);
	rstrm->frag_header = (u_long *)rstrm->out_finger;
	rstrm->out_finger += sizeof(u_long);
	XDR_RPC_DEBUG("  ");
	return (TRUE);
}


/*
 * Internal useful routines
 */
static bool_t
flush_out(rstrm, eor)
	register RECSTREAM *rstrm;
	bool_t eor;
{
	register u_long eormask = (eor == TRUE) ? LAST_FRAG : 0;
	//register u_long len = (u_long)(rstrm->out_finger) - (u_long)(rstrm->frag_header) - sizeof(u_long);
	register u_long len = POINTERS_DIFF(u_long,rstrm->out_finger,rstrm->frag_header) - sizeof(u_long);
	
	XDR_RPC_DEBUG("  ");

	*(rstrm->frag_header) = htonl(len | eormask);
	//len = (u_long)(rstrm->out_finger) - (u_long)(rstrm->out_base);
	len = POINTERS_DIFF(u_long,rstrm->out_finger,rstrm->out_base);
	if ((*(rstrm->writeit))((struct ct_data *)rstrm->tcp_handle, rstrm->out_base, (int)len)!= (int)len)
		return (FALSE);
	rstrm->frag_header = (u_long *)rstrm->out_base;
	rstrm->out_finger = (caddr_t)rstrm->out_base + sizeof(u_long);
	return (TRUE);
}

static bool_t  /* knows nothing about records!  Only about input buffers */
fill_input_buf(rstrm)
	register RECSTREAM *rstrm;
{
	register caddr_t where;
	u_int i;
	register int len;

	where = rstrm->in_base;
	i = (size_t)rstrm->in_boundry % BYTES_PER_XDR_UNIT;
	where += i;
	len = rstrm->in_size - i;
	if ((len = (*(rstrm->readit))((struct ct_data *)rstrm->tcp_handle, where, len)) == -1)
		return (FALSE);
	rstrm->in_finger = where;
	where += len;
	rstrm->in_boundry = where;
	return (TRUE);
}

static bool_t  /* knows nothing about records!  Only about input buffers */
get_input_bytes(rstrm, addr, len)
	register RECSTREAM *rstrm;
	register caddr_t addr;
	register int len;
{
	register int current;

	while (len > 0) {
		//current = (int)rstrm->in_boundry - (int)rstrm->in_finger;
		current = POINTERS_DIFF(int,rstrm->in_boundry,rstrm->in_finger);
		if (current == 0) {
			if (! fill_input_buf(rstrm))
				return (FALSE);
			continue;
		}
		current = (len < current) ? len : current;
		bcopy(rstrm->in_finger, addr, current);
		rstrm->in_finger += current;
		addr += current;
		len -= current;
	}
	return (TRUE);
}

static bool_t  /* next two bytes of the input stream are treated as a header */
set_input_fragment(rstrm)
	register RECSTREAM *rstrm;
{
	u_long header;

	if (! get_input_bytes(rstrm, (caddr_t)&header, sizeof(header)))
		return (FALSE);
	header = (long)ntohl(header);
	rstrm->last_frag = ((header & LAST_FRAG) == 0) ? FALSE : TRUE;
	rstrm->fbtbc = header & (~LAST_FRAG);
	return (TRUE);
}

static bool_t  /* consumes input bytes; knows nothing about records! */
skip_input_bytes(rstrm, cnt)
	register RECSTREAM *rstrm;
	long cnt;
{
	register int current;

	while (cnt > 0) {
		//current = (int)rstrm->in_boundry - (int)rstrm->in_finger;
		current = POINTERS_DIFF(int,rstrm->in_boundry,rstrm->in_finger);
		if (current == 0) {
			if (! fill_input_buf(rstrm))
				return (FALSE);
			continue;
		}
		current = (cnt < current) ? cnt : current;
		rstrm->in_finger += current;
		cnt -= current;
	}
	return (TRUE);
}

static u_int
fix_buf_size(s)
	register u_int s;
{

	if (s < 100)
		s = 4000;
	return (RNDUP(s));
}

MINI_XDR_END_C_DECLS
