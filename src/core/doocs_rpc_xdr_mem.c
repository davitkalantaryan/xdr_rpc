//
// file:			doocs_rpc_xdr_mem.c
//
#include <rpc/wrpc_first_com_include.h>
#include <rpc/types.h>
#include <rpc/xdr.h>


MINI_XDR_BEGIN_C_DECLS

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
	xdrs->x_ops = &xdrmem_ops;
	xdrs->x_private = xdrs->x_base = addr;
	xdrs->x_handy = size;
}


MINI_XDR_END_C_DECLS
