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
#include "xdr_rpc_debug.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <errno.h>
typedef HANDLE	xdr_rpc_thread_t;
#define _rpc_dtablesize(...)	0
#define MyPoll WSAPoll
#else
#include <sys/errno.h>
#include <pthread.h>
typedef pthread_t	xdr_rpc_thread_t;
extern int errno;
#define WSAGetLastError(...)	errno
#define GetCurrentThread	pthread_self
#define MyPoll poll
#endif

//MINI_XDR_EXPORT fd_set svc_fdset;

static xdr_rpc_thread_t  s_svc_run_thread = NULL;
static BOOL svc_run_stop;

int svc_max_pollfd = 0;
struct pollfd* svc_pollfd = NULL;

void svc_getreq_poll(struct pollfd* pfdp, int pollretval);

static VOID NTAPI InterruptFunction(_In_ ULONG_PTR a_parameter)
{
	(void)a_parameter;
}


MINI_XDR_EXPORT void svc_run(void)
{
	int i;
	struct pollfd* my_pollfd = NULL;
	int last_max_pollfd = 0;

	s_svc_run_thread = GetCurrentThread(); // In case of unix GetCurrentThread == pthread_self

#ifndef _WIN32
	sigaction(); // todo:
#endif

	svc_run_stop = TRUE;

	while (svc_run_stop) {
		//readfds = svc_fdset;
		//
		//switch (select(_rpc_dtablesize(), &readfds, NULL, NULL, NULL)) {
		//case -1:
		//	switch (WSAGetLastError()) {
		//	case WSANOTINITIALISED: case WSAEFAULT:
		//		perror("svc_run: - select failed");
		//		return;
		//	default:
		//		break;
		//	}
		//	continue;
		//case 0:
		//	continue;
		//default:
		//	svc_getreqset(&readfds);
		//}

		int max_pollfd = svc_max_pollfd;

		if (max_pollfd == 0) { break; }

		if (last_max_pollfd != max_pollfd){
			struct pollfd* new_pollfd = realloc(my_pollfd, sizeof(struct pollfd) * max_pollfd);

			if (new_pollfd == NULL){
				XDR_RPC_ERR("svc_run: - out of memory");
				break;
			}

			my_pollfd = new_pollfd;
			last_max_pollfd = max_pollfd;
		}


		i = MyPoll(my_pollfd, max_pollfd, -1);

		switch (i){
		case -1:
			if (errno == EINTR)
				continue;
			XDR_RPC_ERR("svc_run: - poll failed. RPC run will be exited");
			break;
		case 0:
			continue;
		default:
			svc_getreq_poll(my_pollfd, i);
			continue;
		}  //  switch (i){

		break;

	}  //  while (svc_run_stop) {
}


MINI_XDR_EXPORT void svc_exit(void)
{
	xdr_rpc_thread_t thisThread = GetCurrentThread(); // In case of unix GetCurrentThread == pthread_self
	svc_run_stop = FALSE;
	if (thisThread != s_svc_run_thread) {
#ifdef _WIN32
		QueueUserAPC(&InterruptFunction, s_svc_run_thread, 0);
#else
		pthread_kill();  // todo:
#endif
	}
}
