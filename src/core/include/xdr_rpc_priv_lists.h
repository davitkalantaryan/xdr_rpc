//
// file:			xdr_rpc_priv_lists.h
// path:			src/core/include/xdr_rpc_priv_lists.h
// created on:		2024 Jan 24
//

#ifndef SRC_CORE_INCLUDE_XDR_RPC_PRIV_LISTS_H
#define SRC_CORE_INCLUDE_XDR_RPC_PRIV_LISTS_H


#include <rpc/wrpc_first_com_include.h>
#include <cinternal/hash/phash.h>
#include <rpc/svc.h>


MINI_XDR_BEGIN_C_DECLS


//MINI_XDR_DLL_PRIVATE extern int g_nLogLevel;
struct SVCXPRTPrivListItem {
	struct SVCXPRTPrivListItem* prev, * next;
	CinternalPHashItem_t	hashIt;
	SVCXPRT* xprt;
};



MINI_XDR_END_C_DECLS


#endif  // #ifndef SRC_CORE_INCLUDE_XDR_RPC_PRIV_LISTS_H
