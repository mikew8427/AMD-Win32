#include "windows.h"
#include <time.h>
#include <sys\types.h>
#include <sys\timeb.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include "schglobal.h"
/**************************************/
/** Define message types to use      **/
/**************************************/
#define MSG_ERR				1
#define MSG_INFO			10
#define MSG_DEBUG			25
#define K1					1024

FILE *W32LOG=NULL;
char W32LOGDIR[K1];
char LogDate[9];
char LogTime[9];
char buf[1024];
char tempname[1024];
char msgbuf [1024];
int	 msgtype;
int  newlog=0;
int OpenLog(char *);
int trim(char *);
void lmsg(int type,const char *format,...);
void SetLogType(int type);
static char * TransType(int type);
CRITICAL_SECTION LOGCS;
//
//  Close the Log file 
//
int CloseLog(void)
{
	if(W32LOG) fclose(W32LOG);
	W32LOG=NULL;
	DeleteCriticalSection(&LOGCS);
return 1;
}
int OpenLog(char *name)
{
int x=1;
DWORD dwt=0,dwl=K1;
HKEY hkey;
long rc=0;


	InitializeCriticalSection(&LOGCS);
	memset(W32LOGDIR,0,sizeof(W32LOGDIR));
	if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_KEY,0,KEY_READ,&hkey))!=ERROR_SUCCESS) strcpy(W32LOGDIR,"C:\\AMD\\");
	else {
		RegQueryValueEx(hkey,REG_AMDLIB,NULL,&dwt,W32LOGDIR,&dwl);
		RegCloseKey(hkey);
		}
	trim(W32LOGDIR);
	strcpy(tempname,W32LOGDIR);
	strcat(tempname,name);
	strcat(tempname,".L00");
	if(newlog) {
		while(!(_access(tempname,0)) ) sprintf(tempname,"%s%s.L%.2d",W32LOGDIR,name,x++);
		}
	W32LOG=fopen(tempname,"w");
	_strdate(LogDate);
	_strtime(LogTime);
	msgtype=MSG_INFO;											// Set for info by default
	lmsg(MSG_INFO,"Log [%s] Opened",name);						// Log it to the system
return 1;
}
int WriteLog(char *line)
{
int clsit=0;

	if(!W32LOG) {clsit=1; W32LOG=fopen(tempname,"a+");}
    _strtime(LogTime);
	sprintf(buf,"[%s] %s\n",LogTime,line);
	fputs(buf,W32LOG);
	if(clsit) CloseLog();
	return 1;
}
int trim(char *data)
{
int x=0;

	x=strlen(data);
	if(x<1) return 0;
	x--;
	while((data[x]<=' ') && x) {data[x]=0; x--;}
	while((data[0])== ' ') strcpy(&data[0],&data[1]);
return x;
}
//
// Build a message from a variable list of args
//
void lmsg(int type,const char *format,...)
{
va_list args;
	
	if (type > msgtype) return;		// check message type (default is MSG_INFO)
	EnterCriticalSection(&LOGCS);
	va_start(args,format);			// Start the args
	vsprintf(buf,format,args);		// Format the buffer
	va_end(args);					// End args
	sprintf(msgbuf,"[%s] %s",TransType(type),buf);
	WriteLog(msgbuf);				// Send it to the log
	LeaveCriticalSection(&LOGCS);
return;
}
void SetLogType(int type)
{
	msgtype=type;					// Reset the message type
}
static char * TransType(int type)
{
	if(type==MSG_INFO)	return ("INFO ");
	if(type==MSG_ERR)	return ("ERROR"); 
	if(type==MSG_DEBUG)	return ("DEBUG");
	return ("UNKN ");
}
char *GetPathInfo(char *name)
{
static char pathbuf[_MAX_PATH+1];
DWORD dwt=0,dwl=_MAX_PATH;
HKEY hkey;
long rc=0;
	if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_KEY,0,KEY_READ,&hkey))!=ERROR_SUCCESS) strcpy(W32LOGDIR,"C:\\AMD\\");
	else {
		RegQueryValueEx(hkey,name,NULL,&dwt,pathbuf,&dwl);
		RegCloseKey(hkey);
		}
return (pathbuf);
}