/*
 *	File: <rpc/wrpc_first_com_include.h> For WINDOWS MFC
 *
 *	Created on: Dec 29, 2015
 *	Author    : Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This is the include file that is included by all gem related headers
 *
 */
#ifndef __wrpc_first_com_include1_h__
#define __wrpc_first_com_include1_h__


#if defined(_MSC_VER)
#pragma include_alias( "rpc.h", "rpc/redirected_win_rpc.h" )
#if (_MSC_VER>1400)
// 'function': incompatible types - from 'pmap *' to 'caddr_t' [CLNT_CALL(client, PMAPPROC_UNSET, xdr_pmap, &parms, xdr_bool, &rslt,tottimeout); ]
//#pragma warning(disable:4133) 
#endif
#endif  // #if defined(_MSC_VER) & (_MSC_VER>1400)

//#include <first_includes/common_include_for_headers.h>

#ifdef _MSC_VER
#define MINI_XDR_DLL_PUBLIC		__declspec(dllexport)
#define MINI_XDR_DLL_PRIVATE
#define MINI_XDR_IMPORT_FROM_DLL	__declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
//#define MINI_XDR_DLL_PUBLIC		__attribute__((visibility("default")))
#define MINI_XDR_DLL_PUBLIC	
#define MINI_XDR_DLL_PRIVATE		__attribute__((visibility("hidden")))
#define MINI_XDR_IMPORT_FROM_DLL
#elif defined(__CYGWIN__)
#define MINI_XDR_DLL_PUBLIC		__attribute__((dllexport))
#define MINI_XDR_DLL_PRIVATE
#define MINI_XDR_IMPORT_FROM_DLL	__attribute__((dllimport))
#elif defined(__MINGW64__) || defined(__MINGW32__)
#define MINI_XDR_DLL_PUBLIC		_declspec(dllexport)
#define MINI_XDR_DLL_PRIVATE
#define MINI_XDR_IMPORT_FROM_DLL	_declspec(dllimport)
#elif defined(__SUNPRO_CC)
#define MINI_XDR_DLL_PUBLIC
#define MINI_XDR_DLL_PRIVATE		__hidden
#define MINI_XDR_IMPORT_FROM_DLL
#endif  // #ifdef _MSC_VER

#if defined(MINI_XDR_COMPILING_SHARED_LIB)
#       define MINI_XDR_EXPORT MINI_XDR_DLL_PUBLIC
#   elif defined(MINI_XDR_USING_STATIC_LIB_OR_OBJECTS)
#       define MINI_XDR_EXPORT
#   else
#       define MINI_XDR_EXPORT MINI_XDR_IMPORT_FROM_DLL
#   endif

#if !defined(MINI_XDR_EXPORT_FNL)
#define MINI_XDR_EXPORT_FNL
#   endif

#if !defined(MINI_XDR_EXPORT_EXTERN)
#define MINI_XDR_EXPORT_EXTERN
#   endif

#if !defined(MINI_XDR_EXPORT_UNIX_LIKE)
#define MINI_XDR_EXPORT_UNIX_LIKE
#   endif

#ifdef __cplusplus
#define MINI_XDR_BEGIN_C_DECLS	extern "C" {
#define MINI_XDR_EXTERN_C		extern "C"
#define	MINI_XDR_END_C_DECLS	}
#else
#define MINI_XDR_BEGIN_C_DECLS
#define MINI_XDR_EXTERN_C
#define	MINI_XDR_END_C_DECLS	
#endif


// Include these headers when everything is already included
#if !defined(__cplusplus)
//#include <rpc/types.h>
//#include <rpc/xdr.h>
//#include <rpc/svc.h>
//#include <rpc/clnt.h>
//#include <process.h>
//#include <rpc/pmap_prot.h>
//#include <rpc/auth_unix.h>
#endif

#ifndef __THROW
#define __THROW
#endif

#endif  // #ifndef __wrpc_first_com_include_h__
