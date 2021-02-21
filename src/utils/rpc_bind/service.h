#ifndef _SERVICE_H
#define _SERVICE_H


#ifdef __cplusplus
extern "C" {
#endif


#define SZAPPNAME            "PORTMAP"
#define SZSERVICENAME        "PortmapService"
#define SZSERVICEDISPLAYNAME "Portmap Service"
// list of service dependencies - "dep1\0dep2\0\0"
#define SZDEPENDENCIES       ""
DWORD WINAPI PortMap();
VOID UpdateStatus(int NewStatus, int Check);
#ifdef __cplusplus
}
#endif

#endif
