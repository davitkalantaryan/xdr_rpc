//
// file:			mini_xdr_rpc_missing_functions.c
//
#include <rpc/wrpc_first_com_include.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <time.h>
#include <rpc/doocs_rpc_unix_like_functions.h>
#include <process.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
#if(_MSC_VER >= 1400)
#define InterlockedCompareExchangePointerNew InterlockedCompareExchangePointer
#else
#define InterlockedCompareExchangePointerNew InterlockedCompareExchange
#endif

#define	bzero(__a_s__,__a_size__)			memset((__a_s__),0,(__a_size__))


MINI_XDR_BEGIN_C_DECLS

#ifdef gettimeofday_is_needed
// todo:
// maybe this function we will need also for DOOCS
// for time being this static
MINI_XDR_EXPORT_UNIX_LIKE
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	static void* spPointer = NULL;
	FILETIME ft;
	unsigned __int64 tmpres = 0;
#if(_MSC_VER >= 1400)
	long lnTzTemp;
	int nldTemp;
#endif

	if (tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		tmpres /= 10;  /*convert into microseconds*/
		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (tz)
	{
		// (setting timezone environmental variable) once
		if (InterlockedCompareExchangePointerNew(&spPointer, (void*)1, NULL) == NULL)
		{
			_tzset();
		}
#if(_MSC_VER >= 1400)
		_get_timezone(&lnTzTemp);
		tz->tz_minuteswest = lnTzTemp / 60;
		_get_daylight(&nldTemp);
		tz->tz_dsttime = nldTemp;
#else
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
#endif
	}

	return 0;
}

#endif  // #ifndef gettimeofday_is_not_needed


// todo:
// maybe this function we will need also for DOOCS
// for time being this static

/*
 * Bind a socket to a privileged IP port
 */
MINI_XDR_EXPORT
int bindresvport(int sd, struct sockaddr_in * sin)
{
	int res;
	static short port;
	struct sockaddr_in myaddr;
	//int my_errno;
	int i;

#define STARTPORT 600
#define ENDPORT (IPPORT_RESERVED - 1)
#define NPORTS	(ENDPORT - STARTPORT + 1)

	if (sin == (struct sockaddr_in *)0) {
		sin = &myaddr;
		bzero(sin, sizeof (*sin));
		sin->sin_family = AF_INET;
	} else if (sin->sin_family != AF_INET) {
		errno = WSAEPFNOSUPPORT;
		return (-1);
	}
	if (port == 0) {
		port = (_getpid() % NPORTS) + STARTPORT;
	}
	res = -1;
	errno = WSAEADDRINUSE;
	for (i = 0; i < NPORTS && res < 0 && errno == WSAEADDRINUSE; i++) {
		sin->sin_port = htons(port++);
		if (port > ENDPORT) {
			port = STARTPORT;
		}
		res = bind(sd, (struct sockaddr*)sin, sizeof(struct sockaddr_in));
		errno = WSAGetLastError();
	}
	return (res);
}


MINI_XDR_END_C_DECLS