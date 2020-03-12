/* @(#)types.h	2.3 88/08/15 4.0 RPCSRC */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
/*      @(#)types.h 1.18 87/07/24 SMI      */

/*
 * Rpc additions to <sys/types.h>
 */
#ifndef __TYPES_RPC_HEADER__
#define __TYPES_RPC_HEADER__

#include <rpc/wrpc_first_com_include.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifndef __const
#define __const const
#endif

/* This needs to be changed to uint32_t in the future */
#if !defined(__rpcprog_t_defined) && !defined(rpcprog_t_defined)
typedef unsigned long rpcprog_t;
#define __rpcprog_t_defined
#define rpcprog_t_defined
#endif  // #if !defined(__rpcprog_t_defined) && !defined(rpcprog_t_defined)
#if !defined(__rpcvers_t_defined) && !defined(rpcvers_t_defined)
typedef unsigned long rpcvers_t;
#define __rpcvers_t_defined
#define rpcvers_t_defined
#endif  // #if !defined(__rpcvers_t_defined) && !defined(rpcvers_t_defined)
#if !defined(__rpcproc_t_defined) && !defined(rpcproc_t_defined)
typedef unsigned long rpcproc_t;
#define __rpcproc_t_defined
#define rpcproc_t_defined
#endif  // #if !defined(__rpcproc_t_defined) && !defined(rpcproc_t_defined)
#if !defined(__rpcprot_t_defined) && !defined(rpcprot_t_defined)
typedef unsigned long rpcprot_t;
#define __rpcprot_t_defined
#define rpcprot_t_defined
#endif  // #if !defined(__rpcprot_t_defined) && !defined(rpcprot_t_defined)
#if !defined(__rpcport_t_defined) && !defined(rpcport_t_defined)
typedef unsigned long rpcport_t;
#define ____rpcport_t_defined_defined
#define __rpcport_t_defined_defined
#endif  // #if !defined(__rpcport_t_defined) && !defined(rpcport_t_defined)

#ifndef N_
#define N_(__a_str__)	((char*)(__a_str__))
#endif

#ifndef bool_t
#define	bool_t	int
#endif
#ifndef enum_t
#define	enum_t	int
#endif
#ifndef FALSE
#define	FALSE	(0)
#endif
#ifndef TRUE
#define	TRUE	(1)
#endif
#ifndef __dontcare__
#define __dontcare__	-1
#endif
#ifndef NULL
#	define NULL 0
#endif

#include <stdlib.h>		/* For malloc decl.  */
#define mem_alloc(bsize)	malloc(bsize)
//#define mem_free(ptr, bsize)	free(ptr)
#ifndef mem_free
#define mem_free(ptr, bsize)	do{free(ptr);ptr=NULL;}while(0)
#endif


#ifndef INADDR_LOOPBACK
#define       INADDR_LOOPBACK         (u_long)0x7F000001
#endif
#ifndef MAXHOSTNAMELEN
#define        MAXHOSTNAMELEN  64
#endif

#ifndef __P
#define __P(__a)	__a
#endif


typedef char* caddr_t;
typedef uint32_t u_int;
typedef uint16_t u_short;
typedef uint8_t u_char;
typedef uint32_t __uint32_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
#ifndef pid_t_defined
#define pid_t_defined
//typedef DWORD pid_t;
typedef uint32_t pid_t;
#endif
typedef unsigned long u_long;


#endif /* ndef __TYPES_RPC_HEADER__ */
