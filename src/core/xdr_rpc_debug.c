//
// file:			xdr_rpc_debug.c
// path:			src/core/xdr_rpc_debug.c
// created on:		2021 Feb 22
//

#include <rpc/wrpc_first_com_include.h>
#include "xdr_rpc_debug.h"

MINI_XDR_BEGIN_C_DECLS

MINI_XDR_DLL_PRIVATE int g_nLogLevel = 0;

MINI_XDR_EXPORT void SetXdrRpcLogLevel(int a_nLogLevel)
{
	g_nLogLevel = a_nLogLevel;
}


MINI_XDR_EXPORT int GetXdrRpcLogLevel(void)
{
	return g_nLogLevel;
}


MINI_XDR_END_C_DECLS
