//
// file:			doocs_rpc_xdr_float.c
//
#include <rpc/wrpc_first_com_include.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <io.h>


#define LAST_FRAG							((u_long)(1 << 31))
#define bcopy(__a_src__,__a_dst__,__a_n__)	memmove((__a_dst__),(__a_src__),(__a_n__))

MINI_XDR_BEGIN_C_DECLS

typedef struct rec_strm {
	caddr_t tcp_handle;
	caddr_t the_buffer;
	/*
	 * out-goung bits
	 */
	int (*writeit)();
	caddr_t out_base;	/* output buffer (points to frag header) */
	caddr_t out_finger;	/* next output position */
	caddr_t out_boundry;	/* data cannot up to this address */
	u_long *frag_header;	/* beginning of curren fragment */
	bool_t frag_sent;	/* true if buffer sent in middle of record */
	/*
	 * in-coming bits
	 */
	int (*readit)();
	u_long in_size;	/* fixed size of the input buffer */
	caddr_t in_base;
	caddr_t in_finger;	/* location of next byte to be had */
	caddr_t in_boundry;	/* can read up to this location */
	long fbtbc;		/* fragment bytes to be consumed */
	bool_t last_frag;
	u_int sendsize;
	u_int recvsize;
} RECSTREAM;

static u_int fix_buf_size(register u_int s);
static bool_t flush_out(register RECSTREAM* rstrm, bool_t eor);
static bool_t  skip_input_bytes(register RECSTREAM* rstrm, long cnt);
static bool_t  set_input_fragment(register RECSTREAM* rstrm);
static bool_t  get_input_bytes(register RECSTREAM* rstrm, register caddr_t addr, register int len);

static bool_t	xdrrec_getlong(XDR* xdrs, register long* lp);
static bool_t	xdrrec_putlong(XDR* xdrs, long* lp);
static bool_t	xdrrec_getbytes(XDR* xdrs, caddr_t addr, u_int len);
static bool_t	xdrrec_putbytes(XDR* xdrs, caddr_t addr, u_int len);
static u_int	xdrrec_getpos(XDR* xdrs);
static bool_t	xdrrec_setpos(XDR* xdrs, u_int pos);
static long* xdrrec_inline(XDR* xdrs, u_int len);
static void	xdrrec_destroy(register XDR* xdrs);

static struct  xdr_ops xdrrec_ops = {
	xdrrec_getlong,
	xdrrec_putlong,
	xdrrec_getbytes,
	xdrrec_putbytes,
	xdrrec_getpos,
	xdrrec_setpos,
	xdrrec_inline,
	xdrrec_destroy
};


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
	int (*readit)();  /* like read, but pass it a tcp_handle, not sock */
	int (*writeit)();  /* like write, but pass it a tcp_handle, not sock */
{
	register RECSTREAM *rstrm =
		(RECSTREAM *)mem_alloc(sizeof(RECSTREAM));

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
	if (rstrm->the_buffer == NULL) {
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
 * The client must tell the package when an end-of-record has occurred.
 * The second paraemters tells whether the record should be flushed to the
 * (output) tcp stream.  (This let's the package support batched or
 * pipelined procedure calls.)  TRUE => immmediate flush to tcp connection.
 */
MINI_XDR_EXPORT
bool_t
xdrrec_endofrecord(xdrs, sendnow)
	XDR *xdrs;
	bool_t sendnow;
{
	register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	register u_long len;  /* fragment length */

	if (sendnow || rstrm->frag_sent || ((rstrm->out_finger + sizeof(u_long)) >=rstrm->out_boundry)) {
		rstrm->frag_sent = FALSE;
		return (flush_out(rstrm, TRUE));
	}
	len = (u_long)(rstrm->out_finger - rstrm->frag_header) - sizeof(u_long);
	*(rstrm->frag_header) = htonl((unsigned long)len | LAST_FRAG);
	rstrm->frag_header = (u_long *)rstrm->out_finger;
	rstrm->out_finger += sizeof(u_long);
	return (TRUE);
}


/*
 * Before reading (deserializing from the stream, one should always call
 * this procedure to guarantee proper record alignment.
 */
MINI_XDR_EXPORT
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


/*//////////// starts statics ////////////////*/
static u_int
xdrrec_getpos(xdrs)
	XDR *xdrs;
{
	register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;
	register __int64 pos;

	pos = _lseek((int)((ptrdiff_t)rstrm->tcp_handle), (long) 0, 1);
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


static void
xdrrec_destroy(xdrs)
	register XDR *xdrs;
{
	register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;

	mem_free(rstrm->the_buffer,
		rstrm->sendsize + rstrm->recvsize + BYTES_PER_XDR_UNIT);
	mem_free((caddr_t)rstrm, sizeof(RECSTREAM));
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
	register u_long len = (u_long)(rstrm->out_finger- rstrm->frag_header- sizeof(u_long));

	*(rstrm->frag_header) = htonl(len | eormask);
	len = (u_long)(rstrm->out_finger- rstrm->out_base);
	if ((*(rstrm->writeit))(rstrm->tcp_handle, rstrm->out_base, (int)len)
		!= (int)len)
		return (FALSE);
	rstrm->frag_header = (u_long *)rstrm->out_base;
	rstrm->out_finger = (caddr_t)rstrm->out_base + sizeof(u_long);
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
	if ((len = (*(rstrm->readit))(rstrm->tcp_handle, where, len)) == -1)
		return (FALSE);
	rstrm->in_finger = where;
	where += len;
	rstrm->in_boundry = where;
	return (TRUE);
}


static bool_t  /* consumes input bytes; knows nothing about records! */
skip_input_bytes(rstrm, cnt)
	register RECSTREAM *rstrm;
	long cnt;
{
	register int current;

	while (cnt > 0) {
		current = (int)(rstrm->in_boundry - rstrm->in_finger);
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


static bool_t  /* knows nothing about records!  Only about input buffers */
get_input_bytes(rstrm, addr, len)
	register RECSTREAM *rstrm;
	register caddr_t addr;
	register int len;
{
	register int current;

	while (len > 0) {
		current = (int)(rstrm->in_boundry - rstrm->in_finger);
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
		}
	return (FALSE);
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
	}
	return (buf);
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
	if ((rstrm->fbtbc >= sizeof(long)) && ((rstrm->in_boundry - buflp) >= sizeof(long))) {
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


static u_int
fix_buf_size(s)
	register u_int s;
{

	if (s < 100)
		s = 4000;
	return (RNDUP(s));
}



MINI_XDR_END_C_DECLS
