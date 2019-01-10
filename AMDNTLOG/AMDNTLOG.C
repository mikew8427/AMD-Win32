#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "msgs.h"
#define LOGTYPE "AMD_NT_MSG"
#define MAX_ARGS 5
int argn;
char *argc[MAX_ARGS];
void BuildArgs(char *cmd);
int GenerateInfo(char *proc,char *msg,DWORD id);
int GenerateWarn(char *proc,char *msg,DWORD id);
int GenerateError(char *proc,char *msg,DWORD id);

int GenerateUserInfo(char *proc,char *msg,DWORD id);
int GenerateUserWarn(char *proc,char *msg,DWORD id);
int GenerateUserError(char *proc,char *msg,DWORD id);

int WINAPI WinMain(	HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdLine,int nCmdShow)
{

		BuildArgs(lpszCmdLine);
		if(argn<3)
			{
			MessageBox(NULL,"Usage: AMDNTLOG [I|W|E] ProcessName Msg | Error Code","AMDNTLOG",MB_OK);
			return 0;
			}
		if(argc[1][0]=='I' || argc[1][0]=='i') GenerateInfo(argc[2],argc[3],AMD_INFO);
		if(argc[1][0]=='W' || argc[1][0]=='w') GenerateWarn(argc[2],argc[3],AMD_WARN);
		if(argc[1][0]=='E' || argc[1][0]=='e') GenerateError(argc[2],argc[3],AMD_ERROR);
		if(stricmp(argc[1],"UE")==0) GenerateUserError(argc[2],argc[3],AMD_USER_ERROR);
		if(stricmp(argc[1],"UI")==0) GenerateUserInfo(argc[2],argc[3],AMD_USER_INFO);
		if(stricmp(argc[1],"UW")==0) GenerateUserWarn(argc[2],argc[3],AMD_USER_WARN);

return 0;
}
void BuildArgs(char *cmd)
{
int x=0;
	argn=1;
	argc[argn]=&cmd[x];
	while(cmd[x]>'\0')
		{
		if(cmd[x]=='\"') {
			x++;
			argc[argn]=&cmd[x];
			while(cmd[x]!='\"')x++;
			cmd[x++]='\0';
			while(cmd[x]==' ')x++;
			argc[++argn]=&cmd[x];
			continue;
		}
		if(cmd[x]==' ')		
			{ 
			cmd[x++]='\0'; 
			while(cmd[x]==' ')x++;
			argc[++argn]=&cmd[x];
			continue;
			}
		x++;
		}
}
//
// Generate an AMD info message to the log
//
int GenerateInfo(char *proc,char *msg,DWORD id)
{
HANDLE  hEventSource;
LPTSTR  lpszStrings[2];

        hEventSource = RegisterEventSource(NULL, LOGTYPE);
        lpszStrings[0] = proc;
        lpszStrings[1] = msg;

        if (hEventSource != NULL) {
            ReportEvent(hEventSource,	// handle of event source
                EVENTLOG_INFORMATION_TYPE, 
                0,						// event category
                id,						// event ID
                NULL,					// current user's SID
                2,						// strings in lpszStrings
                0,						// no bytes of raw data
                lpszStrings,			// array of error strings
                NULL);					// no raw data

            (VOID) DeregisterEventSource(hEventSource);
			return 1;
        }

return 0;
}
//
// Generate an AMD Warning message to the log
//
int GenerateWarn(char *proc,char *msg,DWORD id)
{
HANDLE  hEventSource;
LPTSTR  lpszStrings[2];

        hEventSource = RegisterEventSource(NULL, LOGTYPE);
        lpszStrings[0] = proc;
        lpszStrings[1] = msg;

        if (hEventSource != NULL) {
            ReportEvent(hEventSource,	// handle of event source
                EVENTLOG_WARNING_TYPE, 
                0,						// event category
                id,						// event ID
                NULL,					// current user's SID
                2,						// strings in lpszStrings
                0,						// no bytes of raw data
                lpszStrings,			// array of error strings
                NULL);					// no raw data

            (VOID) DeregisterEventSource(hEventSource);
			return 1;
        }

return 0;
}
//
// Generate an AMD Error message to the log
//
int GenerateError(char *proc,char *msg,DWORD id)
{
HANDLE  hEventSource;
LPTSTR  lpszStrings[2];

        hEventSource = RegisterEventSource(NULL, LOGTYPE);
        lpszStrings[0] = proc;
        lpszStrings[1] = msg;

        if (hEventSource != NULL) {
            ReportEvent(hEventSource,	// handle of event source
                EVENTLOG_ERROR_TYPE, 
                0,						// event category
                id,						// event ID
                NULL,					// current user's SID
                2,						// strings in lpszStrings
                0,						// no bytes of raw data
                lpszStrings,			// array of error strings
                NULL);					// no raw data

            (VOID) DeregisterEventSource(hEventSource);
			return 1;
        }

return 0;
}



//
// Generate an AMD info message to the log
//
int GenerateUserInfo(char *proc,char *msg,DWORD id)
{
HANDLE  hEventSource;
LPTSTR  lpszStrings[2];

        hEventSource = RegisterEventSource(NULL, LOGTYPE);
        lpszStrings[0] = proc;
        lpszStrings[1] = msg;

        if (hEventSource != NULL) {
            ReportEvent(hEventSource,	// handle of event source
                EVENTLOG_INFORMATION_TYPE, 
                0,						// event category
                id,						// event ID
                NULL,					// current user's SID
                2,						// strings in lpszStrings
                0,						// no bytes of raw data
                lpszStrings,			// array of error strings
                NULL);					// no raw data

            (VOID) DeregisterEventSource(hEventSource);
			return 1;
        }

return 0;
}
//
// Generate an AMD Warning message to the log
//
int GenerateUserWarn(char *proc,char *msg,DWORD id)
{
HANDLE  hEventSource;
LPTSTR  lpszStrings[2];

        hEventSource = RegisterEventSource(NULL, LOGTYPE);
        lpszStrings[0] = proc;
        lpszStrings[1] = msg;

        if (hEventSource != NULL) {
            ReportEvent(hEventSource,	// handle of event source
                EVENTLOG_WARNING_TYPE, 
                0,						// event category
                id,						// event ID
                NULL,					// current user's SID
                2,						// strings in lpszStrings
                0,						// no bytes of raw data
                lpszStrings,			// array of error strings
                NULL);					// no raw data

            (VOID) DeregisterEventSource(hEventSource);
			return 1;
        }

return 0;
}
//
// Generate an AMD Error message to the log
//
int GenerateUserError(char *proc,char *msg,DWORD id)
{
HANDLE  hEventSource;
LPTSTR  lpszStrings[2];

        hEventSource = RegisterEventSource(NULL, LOGTYPE);
        lpszStrings[0] = proc;
        lpszStrings[1] = msg;

        if (hEventSource != NULL) {
            ReportEvent(hEventSource,	// handle of event source
                EVENTLOG_ERROR_TYPE, 
                0,						// event category
                id,						// event ID
                NULL,					// current user's SID
                2,						// strings in lpszStrings
                0,						// no bytes of raw data
                lpszStrings,			// array of error strings
                NULL);					// no raw data

            (VOID) DeregisterEventSource(hEventSource);
			return 1;
        }

return 0;
}
