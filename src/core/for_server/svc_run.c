/*********************************************************************
 * RPC for the Windows NT Operating System
 * 1993 by Martin F. Gergeleit
 * Users may use, copy or modify Sun RPC for the Windows NT Operating 
 * System according to the Sun copyright below.
 *
 * RPC for the Windows NT Operating System COMES WITH ABSOLUTELY NO 
 * WARRANTY, NOR WILL I BE LIABLE FOR ANY DAMAGES INCURRED FROM THE 
 * USE OF. USE ENTIRELY AT YOUR OWN RISK!!!
 *********************************************************************/

/* @(#)svc_run.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc_run.c 1.1 87/10/13 Copyr 1984 Sun Micro";
#endif

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

/*
 * This is the rpc server side idle loop
 * Wait for input, call server program.
 */

#include <rpc/wrpc_first_com_include.h>
#include <rpc/svc.h>
#include <cinternal/threading.h>
#include "xdr_rpc_debug.h"
#include "xdr_rpc_priv_lists.h"
#include <stdbool.h>
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <errno.h>
typedef ULONG nfds_t;
#define SleepMsIntr(_x)		SleepEx(_x,TRUE)
#define WaitIntrptInf()		SleepEx(INFINITE,TRUE)
#define MyPoll				WSAPoll
#define ThreadSimpleInterrupt(_thread)	QueueUserAPC(&SimpleInterruptFunction, _thread, 0)
#define PollThreadInterruptForPoll(_arg)	QueueUserAPC(&PollInterruptFunction, s_poll_thread, (ULONG_PTR)((size_t)(_arg)))
#else
#include <sys/errno.h>
#include <pthread.h>
extern int errno;
#define WSAGetLastError(...)	errno
#define GetCurrentThread	pthread_self
#define SleepMsIntr(_x)		usleep(1000*(_x))
#define WaitIntrptInf()		sleep(360000)
#define MyPoll				poll
#endif

#define MAX_POLLFD_SOCKS_COUNT	1024

struct SPollAsyncStruct {
	struct pollfd*	pPollFds;
	nfds_t			nFds;
	int				nRet;
};

static cinternal_thread_t  s_poll_thread = CPPUTILS_NULL;
static cinternal_thread_t  s_svc_run_thread = CPPUTILS_NULL;
static BOOL svc_run_stop;
static bool s_poll_ongoing = false;

static int XdrRpcMultiplexAllSockets(struct pollfd* a_pPollFds);
static cinternal_thread_ret_t PollThreadFunctionStatic(void* a_pArg);
static VOID NTAPI PollInterruptFunction(_In_ ULONG_PTR a_parameter);

static VOID NTAPI SimpleInterruptFunction(_In_ ULONG_PTR a_parameter)
{
	(void)a_parameter;
}


MINI_XDR_EXPORT void svc_run(void)
{
	int nRet;
	struct pollfd* pPollFds;
	cinternal_thread_ret_t threadRet;

	pPollFds = malloc(MAX_POLLFD_SOCKS_COUNT * sizeof(struct pollfd));
	if (!pPollFds) {
		XDR_RPC_ERR("Unable allocate PollFds. Going to exit");
		exit(1);
	}

	s_svc_run_thread = GetCurrentThread();

#ifdef _WIN32
#else
	sigaction(); // todo:
#endif

	svc_run_stop = TRUE;

	nRet = (int)cinternal_thread_create(&s_poll_thread, &PollThreadFunctionStatic, CPPUTILS_NULL);
	if (nRet) {
		s_poll_thread = CPPUTILS_NULL;
		s_svc_run_thread = CPPUTILS_NULL;
		XDR_RPC_ERR("Unable create poll thread. Going to exit");
		exit(1);
	}

	while (svc_run_stop) {

		if (XdrRpcMultiplexAllSockets(pPollFds)) {
			svc_run_stop = 0;
			break;
		}
	}  //  while (svc_run_stop) {

	cinternal_thread_wait_and_clean(&s_poll_thread, &threadRet);
	s_poll_thread = CPPUTILS_NULL;
	s_svc_run_thread = CPPUTILS_NULL;
	free(pPollFds);
}


MINI_XDR_EXPORT void svc_exit(void)
{
	cinternal_thread_t thisThread = GetCurrentThread();
	svc_run_stop = FALSE;
	if (s_svc_run_thread && (thisThread != s_svc_run_thread)) {
		ThreadSimpleInterrupt(s_svc_run_thread);
	}
}


static int XdrRpcMultiplexAllSockets(struct pollfd* a_pPollFds)
{
	// FD_SETSIZE
	bool bPollOngoing = false;
	struct SPollAsyncStruct pollStruct;
	fd_set readfds;
	rpcsocket_t sock;
	struct SVCXPRTPrivListItem * pListItem = s_xprtsListFirst;
	size_t socksCount;
	int nMaxFd = 0;

	if (!pListItem) { return 1; }

	FD_ZERO(&readfds);

	for (socksCount = 0; pListItem && (socksCount < FD_SETSIZE); pListItem = pListItem->next,++socksCount) {
		sock = (rpcsocket_t)pListItem->xprt->xp_sock;
		if (nMaxFd < ((int)sock)) { nMaxFd = (int)sock; }
		FD_SET(sock, &readfds);
	}

	if (pListItem) {
		
		for (socksCount = 0; pListItem && (socksCount < MAX_POLLFD_SOCKS_COUNT); pListItem = pListItem->next, ++socksCount) {
			a_pPollFds[socksCount].fd = pListItem->xprt->xp_sock;
			a_pPollFds[socksCount].events = POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND;
			a_pPollFds[socksCount].revents = 0;
		}

		pollStruct.pPollFds = a_pPollFds;
		pollStruct.nFds = (nfds_t)socksCount;
		pollStruct.nRet = 0;

		PollThreadInterruptForPoll(&pollStruct);
		bPollOngoing = true;
	}

	//if (svc_fdset.fd_count < FD_SETSIZE) {
	//	FD_SET(sock, &svc_fdset);
	//}
	//else {
	//	XDR_RPC_ERR("Number of connections is bigger than FD_SETSIZE(%d), that is currently not supported", FD_SETSIZE);
	//	exit(1);
	//}

	switch (select(nMaxFd+1, &readfds, NULL, NULL, NULL)) {
	case -1:
		switch (WSAGetLastError()) {
		case WSANOTINITIALISED: case WSAEFAULT:
			XDR_RPC_ERR("svc_run: - select failed");
			return 1;
		default:
			SleepMsIntr(10);
			break;
		}
		return 0;
	case 0:
		return 0;
	default:
		svc_getreqset(&readfds);
	}

	if (bPollOngoing) {
		ThreadSimpleInterrupt(s_poll_thread);
	}

	return 0;
}


static cinternal_thread_ret_t PollThreadFunctionStatic(void* a_pArg)
{
	(void)a_pArg;

	while (svc_run_stop) {
		s_poll_ongoing = false;
		WaitIntrptInf();
	}  //  while (svc_run_stop) {

	return (cinternal_thread_ret_t)0;
}


static VOID NTAPI PollInterruptFunction(_In_ ULONG_PTR a_parameter)
{
	struct SPollAsyncStruct* const pPollStruct = (struct SPollAsyncStruct*)((size_t)a_parameter);

	s_poll_ongoing = true;
	pPollStruct->nRet = MyPoll(pPollStruct->pPollFds, pPollStruct->nFds, -1);
	s_poll_ongoing = false;
}
