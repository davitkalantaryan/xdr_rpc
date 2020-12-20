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


/* Structure crudely representing a timezone.
This is obsolete and should never be used.  */
#if !defined(timezone_not_needed) && defined(_WIN32)
#ifdef timezone
#undef timezone
#endif
struct timezone
{
	int tz_minuteswest;		/* Minutes west of GMT.  */
	int tz_dsttime;		/* Nonzero if DST is ever in effect.  */
};
#endif  // #ifndef timezone_not_needed

#ifdef _WIN32
#if defined(gettimeofday_is_needed) || defined(MINI_XDR_COMPILING_SHARED_LIB)
MINI_XDR_EXPORT_UNIX_LIKE int gettimeofday(struct timeval* tv, struct timezone* tz);
#else
// in the case of doocs this is done in the tine
// and unfortunately done with incorrect arguments
struct timeval* gettimeofday(struct timeval* t, struct timezone* tz);
#endif
#endif

#ifdef bindresvport_is_not_needed
#define bindresvport(_sd,_sin)
#else
MINI_XDR_EXPORT int bindresvport(int sd, struct sockaddr_in* sin);
#endif

MINI_XDR_END_C_DECLS


#endif  // #ifndef MINI_XDR_RPC_RPC_UNIX_LIKE_FUNCTIONS_H
