//
// file:			doocs_rpc_unix_like_functions.h
// created on:		2020 Mar 18
//

#ifndef MINI_XDR_RPC_RPC_UNIX_LIKE_FUNCTIONS_H
#define MINI_XDR_RPC_RPC_UNIX_LIKE_FUNCTIONS_H

#include <rpc/wrpc_first_com_include.h>
#include <rpc/types.h>
#ifdef _WIN32
#else
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif

MINI_XDR_BEGIN_C_DECLS


MINI_XDR_EXPORT int bindresvport(int sd, struct sockaddr_in* sin);
MINI_XDR_EXPORT int bindresvport_real(int sd, struct sockaddr_in* sin);

#ifdef bindresvport_real_is_needed
#ifdef bindresvport
#undef bindresvport
#endif
#define bindresvport	bindresvport_real
#endif

MINI_XDR_END_C_DECLS


#endif  // #ifndef MINI_XDR_RPC_RPC_UNIX_LIKE_FUNCTIONS_H
