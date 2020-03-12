//
// file:			doocs_rpc_entry.c
//
#include <rpc/wrpc_first_com_include.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>


MINI_XDR_BEGIN_C_DECLS

#pragma section(".CRT$XCU",read)
#define INITIALIZER_RAW_(f,p) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        static void f(void)

#if defined(_WIN64) || defined(_M_ARM)
#define XDR_MINI_INITIALIZER(f) INITIALIZER_RAW_(f,"")
#else
#define XDR_MINI_INITIALIZER(f) INITIALIZER_RAW_(f,"_")
#endif


static void MiniXdrRpcCleanupRoutine(void)
{
	WSACleanup();
}

XDR_MINI_INITIALIZER(MiniXdrRpcInitializationRoutine)
{
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(2, 2);

	if (WSAStartup(wVersionRequested, &wsaData) != 0){
		return; // we should exit here if static linkage and make flag for librray for failing load
	}

	/* Confirm that the WinSock DLL supports 2.2.		*/
	/* Note that if the DLL supports versions greater	*/
	/* than 2.2 in addition to 2.2, it will still return*/
	/* 2.2 in wVersion since that is the version we		*/
	/* requested.										*/

	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		return ; // we should exit here if static linkage and make flag for librray for failing load
	}

	atexit(&MiniXdrRpcCleanupRoutine);
}


MINI_XDR_END_C_DECLS
