#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include "service.h"

int amdhftp(char *arg);
//***********************************************
//** This Event will be set for Service Stop   **
//***********************************************
HANDLE  hServerStopEvent = NULL;
HANDLE  child			 = NULL;
DWORD	childid			 = 0;

VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
    //************************************************
    //**           Service initialization           **
    //************************************************
    if (!ReportStatusToSCMgr(SERVICE_START_PENDING,NO_ERROR,3000)) return;
	child=CreateThread(NULL,8192,(LPTHREAD_START_ROUTINE)amdhftp,(LPVOID)lpszArgv,0,&childid);
	if(!child) return;
	CloseHandle(child);

    hServerStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);		// Create the Event
    if ( hServerStopEvent == NULL) return;						// Not there get out

    if (!ReportStatusToSCMgr(SERVICE_RUNNING,NO_ERROR,0)) return;
    WaitForSingleObject( hServerStopEvent, INFINITE );			// Wait here for termination
	//
	// Now get rid of the Main Dispatcher
	//
    if (hServerStopEvent) CloseHandle(hServerStopEvent);		// Close this handle
return;
}

/****************************************************/
/**        Signal the service is to end            **/
/****************************************************/
VOID ServiceStop()
{
    if ( hServerStopEvent )
        SetEvent(hServerStopEvent);
}
