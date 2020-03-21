//
// file:			mini_xdr_rpc_src_private.h
// created on:		2020 Mar 19
// created by:		D. Kalantaryan
// 

#ifndef MINI_XDR_RPC_MINI_XDR_RPC_SRC_PRIVATE_H
#define MINI_XDR_RPC_MINI_XDR_RPC_SRC_PRIVATE_H

#include <rpc/wrpc_first_com_include.h>
//#include <WinSock2.h>
//#include <WS2tcpip.h>
//#include <Windows.h>
#include <rpc/clnt.h>
#include <memory.h>
#include <string.h>
#include <process.h>

MINI_XDR_BEGIN_C_DECLS

#define	bzero(__a_s__,__a_size__)			memset((__a_s__),0,(__a_size__))
#define bcopy(__a_src__,__a_dst__,__a_n__)	memmove((__a_dst__),(__a_src__),(__a_n__))

#define getpid								_getpid
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

//errno_t strncpy_s(char* strDest,size_t numberOfElements,const char* strSource,size_t count);
#define strncpy(_dest,_src,_count)	strncpy_s((_dest),(_count),(_src),(_count))
// errno_t strncat_s(char* strDest,size_t numberOfElements,const char* strSource,size_t count);
//char* strncat(char* strDest,const char* strSource,size_t count);
#define strncat(_strDest,_strSource,_count)	strncat_s((_strDest),(_count),(_strSource),(_count))

extern MINI_XDR_DLL_PRIVATE struct rpc_createerr		rpc_createerr;
extern MINI_XDR_DLL_PRIVATE struct opaque_auth			_null_auth;

MINI_XDR_END_C_DECLS


#endif  // #ifndef MINI_XDR_RPC_MINI_XDR_RPC_SRC_PRIVATE_H
