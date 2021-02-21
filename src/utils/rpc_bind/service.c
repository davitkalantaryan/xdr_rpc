#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include "service.h"
#include <conio.h>

enum APP_TYPE{ ERROR_CASE=-1,WINDOWS_APP,CONSOLE_APP, WINDOWS_SAERVICE, RPC_HELP };
static int AppType(int* argc, char*** argv);

SERVICE_STATUS          ssStatus;
SERVICE_STATUS_HANDLE   sshStatusHandle;
//extern BOOL				svc_run_stop;
extern void svc_exit(void);

VOID WINAPI service_ctrl(DWORD dwCtrlCode);
VOID WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
VOID CmdInstallService(void);
VOID CmdRemoveService(void);
int main(int a_argc, char **a_argv)
{
	char** argv = a_argv + 1;
	int argc = a_argc - 1;
	int nAppType;
	BOOL bService;
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)service_main },
		{ NULL, NULL }
	};

	printf("rpc_bind.exe, version 2\n");

    nAppType = AppType(&argc, &argv);

	switch (nAppType)
	{
	case WINDOWS_APP:
		printf("rpc_bind will run without console!\n");
		Sleep(2000);
		FreeConsole();
	case CONSOLE_APP:
		PortMap();
		return 0;
	case WINDOWS_SAERVICE:
		if ((argc > 0) && (((*argv)[0] == '-') || ((*argv)[0] == '/'))){
			if (_stricmp("install", argv[0] + 1) == 0){
				CmdInstallService();
				exit(0);
			}
			if (_stricmp("delete", argv[0] + 1) == 0){
				CmdRemoveService();
				exit(0);
			}
		}
		printf("\nStartServiceCtrlDispatcher being called.\n");
		printf("This may take several seconds.  Please wait.\n");
        FreeConsole();
		bService = StartServiceCtrlDispatcher(dispatchTable);
		return 0;
		break;
    case RPC_HELP:
        printf("usage: %s <mode> [command]\n", a_argv[0]);
        printf("modes: 'c' (console application), 'h' (shows this help), 's' (windows service), 'w' (windows application without console)\n");
        printf("command for case of service: '-install' (install service), '-delete' (delete service)\n");
        break;
	default:
		fprintf(stderr,"unknown APP type!\n");
		break;
	}
}

static int AppType(int* argc, char*** argv)
{
	if ((*argc) < 1) return WINDOWS_APP;

	switch (((*argv)[0])[0])
	{
    case 'c': --(*argc); ++(*argv); return CONSOLE_APP;
	case 'h': --(*argc); ++(*argv); return RPC_HELP;
    case 's': --(*argc); ++(*argv); return WINDOWS_SAERVICE;
    case 'w': --(*argc); ++(*argv); return WINDOWS_APP;
	default: break;
	}

	return ERROR_CASE;
}


void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
    sshStatusHandle = RegisterServiceCtrlHandler( TEXT(SZSERVICENAME), service_ctrl);
    if (!sshStatusHandle) return;
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;
	ssStatus.dwCurrentState = SERVICE_START_PENDING;
	ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ssStatus.dwWin32ExitCode = NO_ERROR;
    ssStatus.dwCheckPoint = 0;
    ssStatus.dwWaitHint = 3000;
	SetServiceStatus( sshStatusHandle, &ssStatus);
    if(PortMap() == 1){
		ssStatus.dwServiceSpecificExitCode = 1;
		ssStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus( sshStatusHandle, &ssStatus);
		return;
	}
    UpdateStatus(SERVICE_STOPPED,0);
    return;

}
VOID UpdateStatus(int NewStatus, int Check)
{
	if(Check < 0)	ssStatus.dwCheckPoint++;
	else			ssStatus.dwCheckPoint = Check;
	if(NewStatus >= 0)	ssStatus.dwCurrentState = NewStatus;
	SetServiceStatus( sshStatusHandle, &ssStatus);
    return;
}

VOID WINAPI service_ctrl(DWORD dwCtrlCode)
{
    switch(dwCtrlCode)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
			svc_exit();
			UpdateStatus(SERVICE_STOP_PENDING,-1);
            return;
        case SERVICE_CONTROL_INTERROGATE:
            break;
        default:
            break;

    }
	UpdateStatus(-1,-1);
}

#pragma warning (disable:4996)

void CmdInstallService(void)
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    TCHAR		szPath[512];

    if ( GetModuleFileName( NULL, szPath, 512 ) == 0 ){
        _tprintf(TEXT("Unable to install %s - 0x%x\n"), TEXT(SZSERVICEDISPLAYNAME), GetLastError());
        return;
    }
    _tcscat(szPath, TEXT(" s"));
    schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if ( schSCManager ){
        schService = CreateService(
            schSCManager,
            TEXT(SZSERVICENAME),
            TEXT(SZSERVICEDISPLAYNAME),
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            szPath,              
            NULL,                
            NULL,
            NULL,
            NULL,
            NULL);
        if ( schService ){
            _tprintf(TEXT("%s installed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            CloseServiceHandle(schService);
        } else _tprintf(TEXT("CreateService failed - 0x%x\n"), GetLastError());
        CloseServiceHandle(schSCManager);
    } else _tprintf(TEXT("OpenSCManager failed - 0x%x\n"), GetLastError());
}

void CmdRemoveService(void)
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if ( schSCManager ){
        schService = OpenService(schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS);
        if (schService){
            if ( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) ){
                _tprintf(TEXT("Stopping %s."), TEXT(SZSERVICEDISPLAYNAME));
                Sleep( 1000 );
                while( QueryServiceStatus( schService, &ssStatus ) ){
                    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING ){
                        _tprintf(TEXT("."));
                        Sleep( 1000 );
                    }else  break;
                }
                if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
                    _tprintf(TEXT("\n%s stopped.\n"), TEXT(SZSERVICEDISPLAYNAME) );
                else
                    _tprintf(TEXT("\n%s failed to stop.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            }
            if( DeleteService(schService) )
                _tprintf(TEXT("%s removed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            else
                _tprintf(TEXT("DeleteService failed - 0x%x\n"), GetLastError());
            CloseServiceHandle(schService);
        }else
            _tprintf(TEXT("OpenService failed - 0x%x\n"), GetLastError());
        CloseServiceHandle(schSCManager);
    }else
        _tprintf(TEXT("OpenSCManager failed - 0x%x\n"), GetLastError());
}
