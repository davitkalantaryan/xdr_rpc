//
// file:			doocs_rpc_xdr_mem.c
//
#include <rpc/wrpc_first_com_include.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <memory.h>
#include <stdint.h>
#include <stddef.h>

#ifndef bcopy
#define bcopy(__a_src__,__a_dst__,__a_n__)	memmove((__a_dst__),(__a_src__),(__a_n__))
//#define	bzero(__a_s__,__a_size__)			memset((__a_s__),0,(__a_size__))
//#define bcmp	memcmp
#endif


MINI_XDR_BEGIN_C_DECLS

static bool_t	xdrmem_getlong(XDR*, register long*);
static bool_t	xdrmem_putlong(XDR*, long*);
static bool_t	xdrmem_getbytes(XDR*, caddr_t, u_int);
static bool_t	xdrmem_putbytes(XDR*, caddr_t, u_int);
static u_int	xdrmem_getpos(XDR*);
static bool_t	xdrmem_setpos(XDR*, u_int);
static long* xdrmem_inline(XDR*, u_int);
static void	xdrmem_destroy(register XDR*);

static struct	xdr_ops xdrmem_ops = {
	xdrmem_getlong,
	xdrmem_putlong,
	xdrmem_getbytes,
	xdrmem_putbytes,
	xdrmem_getpos,
	xdrmem_setpos,
	xdrmem_inline,
	xdrmem_destroy
};

/*
 * The procedure xdrmem_create initializes a stream descriptor for a
 * memory buffer.  
 */
MINI_XDR_EXPORT
void
xdrmem_create(xdrs, addr, size, op)
	register XDR *xdrs;
	caddr_t addr;
	u_int size;
	enum xdr_op op;
{

	xdrs->x_op = op;
	xdrs->x_ops = 0;
	xdrs->x_private = xdrs->x_base = addr;
	xdrs->x_handy = size;
}


static void
xdrmem_destroy(register XDR * xdrs)
	/*XDR *xdrs;*/
{
}

static bool_t
xdrmem_getlong(xdrs, lp)
	XDR *xdrs;
	register long *lp;
{

	if ((xdrs->x_handy -= sizeof(long)) < 0)
		return (FALSE);
	*lp = (long)ntohl((u_long)(*((long *)(xdrs->x_private))));
	xdrs->x_private += sizeof(long);
	return (TRUE);
}

static bool_t
xdrmem_putlong(xdrs, lp)
	XDR *xdrs;
	long *lp;
{

	if ((xdrs->x_handy -= sizeof(long)) < 0)
		return (FALSE);
	*(long *)xdrs->x_private = (long)htonl((u_long)(*lp));
	xdrs->x_private += sizeof(long);
	return (TRUE);
}

static bool_t
xdrmem_getbytes(xdrs, addr, len)
	XDR *xdrs;
	caddr_t addr;
	u_int len;
{

	if ((xdrs->x_handy -= len) < 0)
		return (FALSE);
	bcopy(xdrs->x_private, addr, len);
	xdrs->x_private += len;
	return (TRUE);
}

static bool_t
xdrmem_putbytes(xdrs, addr, len)
	XDR *xdrs;
	caddr_t addr;
	u_int len;
{

	if ((xdrs->x_handy -= len) < 0)
		return (FALSE);
	bcopy(addr, xdrs->x_private, len);
	xdrs->x_private += len;
	return (TRUE);
}

static u_int
xdrmem_getpos(xdrs)
	XDR *xdrs;
{

	return (u_int)(xdrs->x_private - xdrs->x_base);
}

static bool_t
xdrmem_setpos(xdrs, pos)
	XDR *xdrs;
	u_int pos;
{
	register caddr_t newaddr = xdrs->x_base + pos;
	register caddr_t lastaddr = xdrs->x_private + xdrs->x_handy;

	if ((ptrdiff_t)newaddr > (ptrdiff_t)lastaddr)
		return (FALSE);
	xdrs->x_private = newaddr;
	xdrs->x_handy = (int)(lastaddr - newaddr);
	return (TRUE);
}

static long *
xdrmem_inline(xdrs, len)
	XDR *xdrs;
	u_int len;
{
	long *buf = 0;

	if (xdrs->x_handy >= ((int)len)) {
		xdrs->x_handy -= len;
		buf = (long *) xdrs->x_private;
		xdrs->x_private += len;
	}
	return (buf);
}


MINI_XDR_END_C_DECLS
