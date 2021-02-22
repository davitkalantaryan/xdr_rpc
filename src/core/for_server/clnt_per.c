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

/* @(#)clnt_perror.c	2.1 88/07/29 4.0 RPCSRC */
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
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)clnt_perror.c 1.15 87/10/07 Copyr 1984 Sun Micro";
#endif

/*
 * clnt_perror.c
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 */

#include <rpc/wrpc_first_com_include.h>
#include "xdr_rpc_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <string.h>  
#include "mini_xdr_rpc_src_private.h"

#define PER_THREAD_BUFFER_SIZE		1023

MINI_XDR_BEGIN_C_DECLS

MINI_XDR_EXPORT
char*
clnt_sperrno(enum clnt_stat stat);


struct SMemoryListItem {
	struct SMemoryListItem*	next;
}static *s_pMemListFirst,* s_pMemListLast;

static const char *auth_errmsg(enum auth_stat stat);

static HANDLE					s_mutexForList;
static DWORD					s_tlsIndexForBuff;
static char**					s_cpccpSys_errlist;
static int						s_nSys_nerr;


//static char *buf = NULL;

static void clnt_per_cleanup(void)
{
	int i = 0;
	struct SMemoryListItem *pBufItem, * pBufItemNext;
	//WaitForSingleObject(s_mutexForList,INFINITE); // better without mutex
	pBufItem = s_pMemListFirst;
	while(pBufItem){
		pBufItemNext = pBufItem->next;
		//free(pBufItemNext->buff); // not necessary, becuse we did only one allocation
		free(pBufItem);
		pBufItem = pBufItemNext;
	}
	s_mutexForList = NULL;
	//ReleaseMutex(s_mutexForList); // better without mutex

	TlsFree(s_tlsIndexForBuff);
	s_tlsIndexForBuff = 0;
	
	CloseHandle(s_mutexForList);
	s_mutexForList = (HANDLE)0;

	for (; i < s_nSys_nerr; ++i) {
		free(s_cpccpSys_errlist[i]);
	}
	free(s_cpccpSys_errlist);
	s_cpccpSys_errlist = NULL; 
	s_nSys_nerr = 0;
}

// this file is only exception
#pragma warning (disable:4996)


WLAC_INITIALIZER(clnt_per_initialize)
{
	char** ppcNames;

	s_nSys_nerr = sys_nerr;
	ppcNames = sys_errlist;

	if(s_nSys_nerr >0){
		int i = 0;
		s_cpccpSys_errlist = (char**)malloc(sizeof(char*)*s_nSys_nerr);
		for(;i< s_nSys_nerr;++i){
			s_cpccpSys_errlist[i] = _strdup(ppcNames[i]);
		}
	}

	s_mutexForList = CreateMutex(NULL, FALSE, NULL);
	s_tlsIndexForBuff = TlsAlloc();
	s_pMemListLast = s_pMemListFirst = NULL;
	atexit(clnt_per_cleanup);
}

#undef sys_nerr
#define sys_nerr		s_nSys_nerr

#undef sys_errlist
#define sys_errlist		s_cpccpSys_errlist

static char *
_buf2(void)
{
	char* pcBuff = (char*)TlsGetValue(s_tlsIndexForBuff);

	if (MINI_XDR_UNLIKELY(!pcBuff)){
		struct SMemoryListItem* pItem = (struct SMemoryListItem*)malloc(sizeof(struct SMemoryListItem)+ PER_THREAD_BUFFER_SIZE+1);
		if(MINI_XDR_LIKELY(pItem)){
			pItem->next = NULL;
			pcBuff = ((char*)pItem)+sizeof(struct SMemoryListItem);
			WaitForSingleObject(s_mutexForList, INFINITE);
			if(s_pMemListLast){
				s_pMemListLast->next = pItem;
			}
			else{
				s_pMemListFirst=pItem;
			}
			s_pMemListLast = pItem;
			ReleaseMutex(s_mutexForList);
			TlsSetValue(s_tlsIndexForBuff,pcBuff);
		}

	}
	return (pcBuff);
}


/*
 * Print reply error info
 */
MINI_XDR_EXPORT
char *
clnt_sperror(rpch, s)
	CLIENT *rpch;
	char *s;
{
	struct rpc_err e;
	//void clnt_perrno();
	const char *err;
	char *str = _buf2();
	char *strstart = str;
	size_t unBufferSize = PER_THREAD_BUFFER_SIZE;
	size_t unHandled;

	if (str == 0)
		return (0);
	CLNT_GETERR(rpch, &e);

	unHandled = (size_t) snprintf(str, unBufferSize, "%s: ", s);
	str += unHandled;
	unBufferSize -= unHandled;
	if (!unBufferSize) { return NULL; }

	(void) strncpy(str,clnt_sperrno(e.re_status),unBufferSize);
	unHandled = strlen(str);
	str += unHandled;
	unBufferSize -= unHandled;
	if (!unBufferSize) { return NULL; }

	switch (e.re_status) {
	case RPC_SUCCESS:
	case RPC_CANTENCODEARGS:
	case RPC_CANTDECODERES:
	case RPC_TIMEDOUT:
	case RPC_PROGUNAVAIL:
	case RPC_PROCUNAVAIL:
	case RPC_CANTDECODEARGS:
	case RPC_SYSTEMERROR:
	case RPC_UNKNOWNHOST:
	case RPC_UNKNOWNPROTO:
	case RPC_PMAPFAILURE:
	case RPC_PROGNOTREGISTERED:
	case RPC_FAILED:
		break;

	case RPC_CANTSEND:
	case RPC_CANTRECV:
#ifdef _WIN32
		if (e.re_errno < sys_nerr) {
#endif
			unHandled = (size_t)snprintf(str,unBufferSize,"; errno = %s", sys_errlist[e.re_errno]);
#ifdef _WIN32
		}
		else{
			unHandled = (size_t)snprintf(str, unBufferSize, "Error %d, ", e.re_errno);
		}
#endif
		//str += strlen(str);
		str += unHandled;
		unBufferSize -= unHandled;
		if (!unBufferSize) { return NULL; }
		break;

	case RPC_VERSMISMATCH:
		unHandled = (size_t)snprintf(str, unBufferSize,"; low version = %lu, high version = %lu",e.re_vers.low, e.re_vers.high);
		//str += strlen(str);
		str += unHandled;
		unBufferSize -= unHandled;
		if (!unBufferSize) { return NULL; }
		break;

	case RPC_AUTHERROR:
		err = auth_errmsg(e.re_why);
		unHandled = (size_t)snprintf(str, unBufferSize,"; why = ");
		//str += strlen(str);
		str += unHandled;
		unBufferSize -= unHandled;
		if (!unBufferSize) { return NULL; }
		if (err != NULL) {
			unHandled = (size_t)snprintf(str, unBufferSize,"%s",err);
		} else {
			unHandled = (size_t)snprintf(str, unBufferSize,"(unknown authentication error - %d)",(int) e.re_why);
		}
		//str += strlen(str);
		str += unHandled;
		unBufferSize -= unHandled;
		break;

	case RPC_PROGVERSMISMATCH:
		unHandled = (size_t)snprintf(str, unBufferSize,"; low version = %lu, high version = %lu",e.re_vers.low, e.re_vers.high);
		//str += strlen(str);
		str += unHandled;
		unBufferSize -= unHandled;
		break;

	default:	/* unknown */
		unHandled = (size_t)snprintf(str,unBufferSize,"; s1 = %lu, s2 = %lu",e.re_lb.s1, e.re_lb.s2);
		//str += strlen(str);
		str += unHandled;
		unBufferSize -= unHandled;
		break;
	}
	(void)snprintf(str, unBufferSize, "\n");
	return(strstart) ;
}


MINI_XDR_EXPORT
void
clnt_perror(rpch, s)
	CLIENT *rpch;
	char *s;
{
	(void) fprintf(stderr,"%s",clnt_sperror(rpch,s));
}


struct rpc_errtab {
	enum clnt_stat status;
	char *message;
};

static struct rpc_errtab  rpc_errlist[] = {
	{ RPC_SUCCESS,
		"RPC: Success" },
	{ RPC_CANTENCODEARGS,
		"RPC: Can't encode arguments" },
	{ RPC_CANTDECODERES,
		"RPC: Can't decode result" },
	{ RPC_CANTSEND,
		"RPC: Unable to send" },
	{ RPC_CANTRECV,
		"RPC: Unable to receive" },
	{ RPC_TIMEDOUT,
		"RPC: Timed out" },
	{ RPC_VERSMISMATCH,
		"RPC: Incompatible versions of RPC" },
	{ RPC_AUTHERROR,
		"RPC: Authentication error" },
	{ RPC_PROGUNAVAIL,
		"RPC: Program unavailable" },
	{ RPC_PROGVERSMISMATCH,
		"RPC: Program/version mismatch" },
	{ RPC_PROCUNAVAIL,
		"RPC: Procedure unavailable" },
	{ RPC_CANTDECODEARGS,
		"RPC: Server can't decode arguments" },
	{ RPC_SYSTEMERROR,
		"RPC: Remote system error" },
	{ RPC_UNKNOWNHOST,
		"RPC: Unknown host" },
	{ RPC_UNKNOWNPROTO,
		"RPC: Unknown protocol" },
	{ RPC_PMAPFAILURE,
		"RPC: Port mapper failure" },
	{ RPC_PROGNOTREGISTERED,
		"RPC: Program not registered"},
	{ RPC_FAILED,
		"RPC: Failed (unspecified error)"}
};


/*
 * This interface for use by clntrpc
 */
MINI_XDR_EXPORT
char *
clnt_sperrno(enum clnt_stat stat)
{
	int i;

	for (i = 0; i < sizeof(rpc_errlist)/sizeof(struct rpc_errtab); i++) {
		if (rpc_errlist[i].status == stat) {
			return (rpc_errlist[i].message);
		}
	}
	return ("RPC: (unknown error code)");
}


MINI_XDR_EXPORT
void
clnt_perrno(num)
	enum clnt_stat num;
{
	(void) fprintf(stderr,"%s",clnt_sperrno(num));
}


MINI_XDR_EXPORT
char *
clnt_spcreateerror(s)
	char *s;
{
	size_t unBufferSize = PER_THREAD_BUFFER_SIZE;
	size_t unHandled;
	char *str = _buf2();
	char* strstart = str;

	if (!str)return(0);

	snprintf(str, unBufferSize, "%s: ", s);
	//str += unHandled;
	//unBufferSize -= unHandled;
	//if (!unBufferSize) { return NULL; }
	(void) strncat(str,clnt_sperrno(rpc_createerr.cf_stat),unBufferSize);
	//unHandled = strlen(str);
	//str += unHandled;
	//unBufferSize -= unHandled;
	//if (!unBufferSize) { return NULL; }
	switch (rpc_createerr.cf_stat) {
	case RPC_PMAPFAILURE:
		(void) strncat(str," - ", unBufferSize);
		(void) strncat(str, clnt_sperrno(rpc_createerr.cf_error.re_status), unBufferSize);
		break;

	case RPC_SYSTEMERROR:
		(void) strncat(str, " - ", unBufferSize);
		if (rpc_createerr.cf_error.re_errno > 0 && rpc_createerr.cf_error.re_errno < sys_nerr)
			(void) strncat(str,sys_errlist[rpc_createerr.cf_error.re_errno], unBufferSize);
		else {
			unHandled = strlen(str);
			str += unHandled;
			unBufferSize -= unHandled;
			if (!unBufferSize) { return NULL; }
			(void)snprintf(str, unBufferSize,"Error %d", rpc_createerr.cf_error.re_errno);
		}
		break;
	}
	(void) strncat(str,"\n", unBufferSize);
	return (str);
}


MINI_XDR_EXPORT
void
clnt_pcreateerror(s)
	char *s;
{
	(void) fprintf(stderr,"%s",clnt_spcreateerror(s));
}


struct auth_errtab {
	enum auth_stat status;
	char *message;
};

static struct auth_errtab auth_errlist[] = {
	{ AUTH_OK,
		"Authentication OK" },
	{ AUTH_BADCRED,
		"Invalid client credential" },
	{ AUTH_REJECTEDCRED,
		"Server rejected credential" },
	{ AUTH_BADVERF,
		"Invalid client verifier" },
	{ AUTH_REJECTEDVERF,
		"Server rejected verifier" },
	{ AUTH_TOOWEAK,
		"Client credential too weak" },
	{ AUTH_INVALIDRESP,
		"Invalid server verifier" },
	{ AUTH_FAILED,
		"Failed (unspecified error)" },
};

static const char *
auth_errmsg(stat)
	enum auth_stat stat;
{
	int i;

	for (i = 0; i < sizeof(auth_errlist)/sizeof(struct auth_errtab); i++) {
		if (auth_errlist[i].status == stat) {
			return(auth_errlist[i].message);
		}
	}
	return(NULL);
}

/*//////////////////////////////////////////////////////////////////////*/
MINI_XDR_DLL_PRIVATE struct rpc_createerr		rpc_createerr;


MINI_XDR_END_C_DECLS
