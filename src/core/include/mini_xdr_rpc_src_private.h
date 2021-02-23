//
// file:			mini_xdr_rpc_src_private.h
// created on:		2020 Mar 19
// created by:		D. Kalantaryan
// 

#ifndef MINI_XDR_RPC_MINI_XDR_RPC_SRC_PRIVATE_H
#define MINI_XDR_RPC_MINI_XDR_RPC_SRC_PRIVATE_H

#ifndef XDR_RPC_REGISTER
#ifdef __cplusplus
#define XDR_RPC_REGISTER
#else
#define XDR_RPC_REGISTER	 register
#endif
#endif

#include <rpc/wrpc_first_com_include.h>
//#include <WinSock2.h>
//#include <WS2tcpip.h>
//#include <Windows.h>
#include <rpc/clnt.h>
#include <memory.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
typedef int sndrcv_size_t;
typedef int sndrcv_ret_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#if defined(EMSCRIPTEN) || defined(__EMSCRIPTEN__)
//#define closesocket(_sock)	do{shutdown(  (_sock),SHUT_RDWR  );close((_sock));}while(0)
#define closesocket(_sock)	shutdown(  (_sock),SHUT_RDWR  );close((_sock))
#else
#define closesocket	close
#endif
typedef size_t sndrcv_size_t;
typedef ssize_t sndrcv_ret_t;
#endif


#define XDR_RPC_UNUSED(_var) (void)(_var)


MINI_XDR_BEGIN_C_DECLS

extern MINI_XDR_DLL_PRIVATE struct rpc_createerr		rpc_createerr;

/* Structure crudely representing a timezone.
This is obsolete and should never be used.  */
#if !defined(timezone_defined) && !defined(timezone_not_needed)
#define timezone_defined
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

#define	bzero(__a_s__,__a_size__)			memset((__a_s__),0,(__a_size__))
#define bcopy(__a_src__,__a_dst__,__a_n__)	memmove((__a_dst__),(__a_src__),(__a_n__))

#ifdef _MSC_VER
#define getpid								_getpid
#endif
#define POINTERS_DIFF(_type,_pbig,_psmall)		(_type)( ((caddr_t)(_pbig))-((caddr_t)(_psmall)) )

#pragma section(".CRT$XCU",read)
#define INITIALIZER_RAW_(f,p) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        static void f(void)
#if defined(_WIN64) || defined(_M_ARM)
#define WLAC_INITIALIZER(f) INITIALIZER_RAW_(f,"")
#else
#define WLAC_INITIALIZER(f) INITIALIZER_RAW_(f,"_")
#endif

#define MINI_XDR_LIKELY(_cond)		(_cond)
#define MINI_XDR_UNLIKELY(_cond)	(_cond)

MINI_XDR_DLL_PRIVATE struct opaque_auth _null_auth;

#ifdef _MSC_VER
// Microsoft versions of safe string functions
//errno_t strncpy_s(char* strDest,size_t numberOfElements,const char* strSource,size_t count);
#define strncpy(_dest,_src,_count)	strncpy_s((_dest),(_count),(_src),(_count))
// errno_t strncat_s(char* strDest,size_t numberOfElements,const char* strSource,size_t count);
//char* strncat(char* strDest,const char* strSource,size_t count);
#define strncat(_strDest,_strSource,_count)	strncat_s((_strDest),(_count),(_strSource),(_count))
#endif


MINI_XDR_END_C_DECLS


#endif  // #ifndef MINI_XDR_RPC_MINI_XDR_RPC_SRC_PRIVATE_H
