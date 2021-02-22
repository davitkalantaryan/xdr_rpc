/*
 *	File: <rpc/wrpc_first_com_include.h> For WINDOWS MFC
 *
 *	Created on: Dec 29, 2015
 *	Author    : Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This is the include file that is included by all gem related headers
 *
 */
#ifndef INCLUDE_RPC_WRPC_FIRST_COM_INCLUDE_H
#define INCLUDE_RPC_WRPC_FIRST_COM_INCLUDE_H

#include "xdr_rpc_internal.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

MINI_XDR_BEGIN_C_DECLS

#ifdef _WIN32
typedef	SOCKET	rpcsocket_t;
#else
typedef	int	rpcsocket_t;
#endif

MINI_XDR_END_C_DECLS

#endif  // #ifndef INCLUDE_RPC_WRPC_FIRST_COM_INCLUDE_H
