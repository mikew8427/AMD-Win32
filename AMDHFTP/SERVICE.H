#ifndef _SERVICE_H
#define _SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif

//**************************************************************************
//** Define Executable Name, Service Name, Display Name, and dependencies **
//**************************************************************************
#define SZAPPNAME            "AMDHFTP"
#define SZSERVICENAME        "AMDHFTP"
#define SZSERVICEDISPLAYNAME "AMD FTP SERVER"
#define SZDEPENDENCIES       "tcpip\0\0"
#define MAXTASK				 64

//**************************************************************************
//**                  Define the routines that will be needed             **
//**************************************************************************
VOID ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
VOID ServiceStop();
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void AddToMessageLog(LPTSTR lpszMsg);
void StartTalk(char *port);
HANDLE AmdTask[MAXTASK];
SOCKET AmdSock[MAXTASK];
SOCKADDR_IN AmdAddr[MAXTASK];
CRITICAL_SECTION TASKS;
CRITICAL_SECTION DATAO;
//***************************************************************************
#ifdef __cplusplus
}
#endif

#endif
