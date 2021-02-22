//
// file:			xdr_rpc_debug.h
// path:			src/core/include/xdr_rpc_debug.h
// created on:		2021 Feb 22
//

#ifndef SRC_CORE_INCLUDE_XR_RPC_DEBUG_H
#define SRC_CORE_INCLUDE_XR_RPC_DEBUG_H


#include <rpc/wrpc_first_com_include.h>
#include <stdio.h>
#include <string.h>


MINI_XDR_BEGIN_C_DECLS


MINI_XDR_DLL_PRIVATE extern int g_nLogLevel;

#ifdef _WIN32
#define	_rpc_dtablesize(...)			0
#define FILE_DELIM_CHAR					'\\'
#define COMMON_SOCK_ERRNO				WSAGetLastError()
#define COMMON_ERRNO					GetLastError()
#define RecvOrRead(_sock,_buf,_len)		recv((_sock),(_buf),(_len),0)
#define SendOrWrite(_sock,_buf,_len)	send((_sock),(_buf),(_len),0)
typedef	SOCKET	rpcsocket_t;
#else
#define	closesocket						close
#define INVALID_SOCKET					-1
#define	WSAEINTR						EINTR
#define WSAEWOULDBLOCK					EWOULDBLOCK
#define WSAEAFNOSUPPORT					EAFNOSUPPORT

#define FILE_DELIM_CHAR					'/'
#define COMMON_SOCK_ERRNO				errno
#define COMMON_ERRNO					errno
#define RecvOrRead						read
#define SendOrWrite						write
typedef	int	rpcsocket_t;
#endif

#define FILE_NAME_FROM_PATH(_path)	( strrchr((_path),FILE_DELIM_CHAR) ? (strrchr((_path),FILE_DELIM_CHAR)+1) : (_path)  )

#define XDR_RPC_LOG_RAW(_logPipe,_logLevel,...)	\
	do{	\
		if((_logLevel)<=g_nLogLevel){	\
			fprintf((_logPipe),"fl:%s,ln:%d,fn:%s => ",FILE_NAME_FROM_PATH(__FILE__),__LINE__,__FUNCTION__);	\
			fprintf((_logPipe),__VA_ARGS__);	\
			fprintf((_logPipe),"\n");	\
		}	\
	}while(0)


#define XDR_RPC_LOG(_logLevel,...)		XDR_RPC_LOG_RAW(stdout,_logLevel,__VA_ARGS__)
#define XDR_RPC_ERR(...)				XDR_RPC_LOG_RAW(stderr,0,__VA_ARGS__)
#define XDR_RPC_DEBUG(...)				XDR_RPC_LOG_RAW(stdout,2,__VA_ARGS__)


MINI_XDR_END_C_DECLS


#endif  // #ifndef SRC_CORE_INCLUDE_XR_RPC_DEBUG_H
