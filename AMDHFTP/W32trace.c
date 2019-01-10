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
#define DTBUF				16

FILE *W32LOG=NULL;
char W32LOGDIR[K1];
char LogDate[DTBUF];
char LogTime[DTBUF];
char buf[K1];
char tempname[K1];
char msgbuf [K1];
int	 msgtype;
int  newlog=0;
int OpenLog(char *);
int trim(char *);
void lmsg(int type,const char *format,...);
void SetLogType(int type);
static char * TransType(int type);
CRITICAL_SECTION LOGCS;
void CheckEvent(char *buf);

//
//  Close the Log file 
//
int CloseLog(void)
{
	if(W32LOG) fclose(W32LOG);
	W32LOG=NULL;
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
	if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_KEY,0,KEY_READ,&hkey))!=ERROR_SUCCESS) {
		rc=K1;
		GetCurrentDirectory(rc,W32LOGDIR);
		strcat_s(W32LOGDIR,(sizeof(W32LOGDIR)-1), "\\");
		}
	else {
		RegQueryValueEx(hkey,REG_AMDLIB,NULL,&dwt,W32LOGDIR,&dwl);
		RegCloseKey(hkey);
		}
	x=trim(W32LOGDIR);
	strcpy_s(tempname,K1,W32LOGDIR);
	strcat_s(tempname,K1,name);
	strcat_s(tempname, K1, ".L00");
	if(newlog) {
		while(!(_access(tempname,0)) ) sprintf_s(tempname,(sizeof(tempname)-1),"%s%s.L%.2d",W32LOGDIR,name,x++);
		}
	W32LOG=fopen(tempname,"w");
	_strdate_s(LogDate,DTBUF);
	_strtime_s(LogTime,DTBUF);
	msgtype=MSG_INFO;											// Set for info by default
return 1;
}
int WriteLog(char *line)
{
int clsit=0;

	if(!W32LOG) W32LOG=fopen(tempname,"a+");
    _strtime_s(LogTime,DTBUF);
	_strdate_s(LogDate, DTBUF);
	sprintf_s(buf,(sizeof(buf)-1),"[%s %s] %s\n",LogDate,LogTime,line);
	if(W32LOG) fputs(buf,W32LOG);
	CloseLog();
	return 1;
}
int trim(char *data)
{
int x=0;

	x=strlen(data);
	if(x<1) return 0;
	x--;
	while((data[x]<=' ') && x) {data[x]=0; x--;}
	if(x==0) data[0]='\0';
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
	vsprintf_s(buf,(sizeof(buf)-1),format,args);		// Format the buffer
	va_end(args);					// End args
	sprintf_s(msgbuf,(sizeof(msgbuf)-1),"[%s] %s",TransType(type),buf);
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
char base[16]= "C:\\AMD\\";

	if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_KEY,0,KEY_READ,&hkey))!=ERROR_SUCCESS) strcpy_s(W32LOGDIR,strlen(base),base);
	else {
		RegQueryValueEx(hkey,name,NULL,&dwt,pathbuf,&dwl);
		RegCloseKey(hkey);
		}
return (pathbuf);
}