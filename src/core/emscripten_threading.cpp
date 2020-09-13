//
// file:		emscripten_threading.cpp
// created on:	2020 Sep 13
//

#include "mini_xdr_rpc_src_private.h"
#include <pthread.h>

MINI_XDR_BEGIN_C_DECLS

pthread_t pthread_self_emscr(void)
{
#ifdef pthread_self
#undef pthread_self
#endif
	return pthread_self();
}

MINI_XDR_END_C_DECLS
