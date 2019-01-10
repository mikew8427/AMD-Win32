#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
// CLASS=Security APP=Security OUTPUT=<AMD_Data>ntlog.csv APPEND
#define LOGTYPE "AMD_NT_MSG"
#define MAX_ARGS		25
#define MAX_STRINGS		50
#define MAX_STRING_LEN	128
#define REG_KEY			"SYSTEM\\CurrentControlSet\\Services\\EventLog"
#define MAXMSGSIZE		8192
#define CLASS_APP		"Application"
#define CLASS_SEC		"Security"
#define CLASS_SYS		"System"

int					argn;										// Number of arguments passed
char				*argc[MAX_ARGS];							// pointers to the args
char				*lpargs[MAX_STRINGS];						// Arguments for the current message
char				*p;											// pointer into the event block struct
HANDLE				hEventLog;									// handle for the event log
HANDLE				hModHandle;									// handle to the message dll or exe
char				*szEvBuf;									// pointer for readeventlog
EVENTLOGRECORD		*pevlr;										// Current event record
DWORD				dwRead;										// Number of bytes read
DWORD				dwNeeded;									// if we are short what we need
DWORD				dwBufSize = 12288;							// Get 12k buffer
BOOL				rc;											// rc used for api
BOOL				ec;											// getlasterror rc
char				Class[32]="Application";					// Class Name 
char				AppName[256]=LOGTYPE;						// App Name
char				Computer[256];								// Computer Name
char				MsgFile[_MAX_PATH+1];						// Holds Message File
char				MsgFiles[10][_MAX_PATH+1];					// All the File Names
int					NumOfFiles;									// Number of files in MsgFiles
LPSTR				Message;									// Holds Message
char				Strings[MAX_STRINGS][MAX_STRING_LEN];		// Hold the argument data
FILE *out;														// Output file pointer
char				OutName[_MAX_PATH+1]="";					// Output file name
int					reccount=0;									// Records out to file
int					Clear=0;									// Clear Event Log when Done?
int					Append=0;									// Append to Exisiting FIle
char				Level[32]="Warning";						// Char Level for message filter
char				Msgbuf[4096];

void BuildArgs(char *cmd);										// Parse input args
char * GetString( char *s, char *p, DWORD size );				// Get a string from p and increment
int GetAppMsgFile(char *Class,char *app,char *fname);			// Copy out the Msg FIle Name
int GetParmMsgFile(char *Class,char *app,char *fname);			// Copy out the Msg FIle Name
int CopyStrings(char *p, int count);							// Copy over all the Argument strings
void LoadArgs(void);											// Find args and set global vars
int UpdateMessageText(char *Msg,int len);						// Clear out Bogus chars from message
int WriteMsgFile(EVENTLOGRECORD *e);							// Write info to log
char *SetEventType(WORD type);									// Make number a English
int LevelOk(char *class,WORD level,char *requested);			// Check to see if Level is OK
int FormatParmMessage(char *Class,char *AppName,char *Message);
int trim(char *data);

int WINAPI WinMain(	HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdLine,int nCmdShow)
{
int x;
DWORD fmtlength;

		BuildArgs(lpszCmdLine);									// Parse them out
		LoadArgs();												// Load up Global Variales
		if(OutName[0]<' ') return 16;							// No File no Run
		if(!Append) DeleteFile(OutName);						// Remove Old File if not Appending
		hEventLog = OpenEventLog( NULL,AppName);				// Open by name
		if (!hEventLog) {
			ExitProcess(8);
			}
	    szEvBuf = (char *) malloc( dwBufSize );					// Alloc the read buffer
		for(x=0;x<MAX_STRINGS;x++) lpargs[x]=Strings[x];		// Set lpargs to strings Address

	    while (TRUE) 
		{ 
		    rc = ReadEventLog(hEventLog, 
                        EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ, 
                        0, 
                        (EVENTLOGRECORD *) szEvBuf, 
                        dwBufSize, 
                        &dwRead, 
                        &dwNeeded); 
 
			if (!rc) 
				{
				rc=GetLastError();
				if(rc==38) break;										// End of File
				exit(rc);
	            } 
 			if (dwRead == 0) break;  
			p = szEvBuf; 
 
			do { 
				fmtlength=0;											// Set value each time
				pevlr = (EVENTLOGRECORD *) p;							// Save the starting point
				if(!LevelOk(Class,pevlr->EventType,Level))				// Filter on Event
					{
					dwRead -= pevlr->Length;							// Subtract whats left
					p = (char *) ((DWORD)pevlr + pevlr->Length);		// move to next record
					continue;
					}
	            p = (char *) ((DWORD)pevlr + sizeof(EVENTLOGRECORD));	// Point to end of structure
				p = GetString(AppName,p,sizeof(AppName));				// Save Application Name
				p = GetString(Computer,p,sizeof(Computer));				// Get this machines name
	            p = (char*)pevlr+pevlr->StringOffset;					// Offset of strings to use
				CopyStrings(p,pevlr->NumStrings);						// Copy strings to Global buffer
				GetAppMsgFile(Class,AppName,MsgFile);					// Load in the file
				//
				// Load up Main Module and and process this (usually ok)
				//
				for(x=0;x<NumOfFiles;x++)
					{
					hModHandle=LoadLibraryEx(MsgFiles[x],NULL,LOAD_WITH_ALTERED_SEARCH_PATH);		// Get handle for msg file
					fmtlength=FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							 FORMAT_MESSAGE_ARGUMENT_ARRAY | 60,  
							 hModHandle,
							 pevlr->EventID,
							 LANG_USER_DEFAULT,  
							 (LPTSTR) &Message, 
							 MAXMSGSIZE,		 
							 lpargs);									// Format this Message
					if(fmtlength) break;								// No Need to continue
					}
				if(fmtlength)
					{
					FormatParmMessage(Class,AppName,Message);			// Resolve into MsgBuf
					UpdateMessageText(Msgbuf,strlen(Msgbuf));			// Clear out CRLF and ;
					WriteMsgFile(pevlr);								// Merge Global & event data
					LocalFree((HLOCAL)Message);							// Free the storage you all
					}
				dwRead -= pevlr->Length;								// Subtract whats left
				p = (char *) ((DWORD)pevlr + pevlr->Length);			// move to next record
				} while ( dwRead > 0 );									// You all come back ya-hear!
		} 
   if(Clear) ClearEventLog(hEventLog,NULL);								// Clear was called for
   free( szEvBuf ); 
   CloseEventLog( hEventLog ); 
return 0;
}
//
// Beakup I say Break up the input args into managable parts
//
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
// Place args in Global Vars
//
void LoadArgs(void)
{
int x=1;
	while(x<=argn)
		{
		if (strnicmp(argc[x],"Class=",6)==0) {strcpy(Class,&argc[x][6]);   x++; continue;}
		if (strnicmp(argc[x],"App=",4)==0)   {strcpy(AppName,&argc[x][4]); x++; continue;}
		if (strnicmp(argc[x],"Clear",5)==0)  {Clear=1;					   x++; continue;}
		if (strnicmp(argc[x],"Append",5)==0) {Append=1;					   x++; continue;}
		if (strnicmp(argc[x],"OutPut=",7)==0){strcpy(OutName,&argc[x][7]); x++; continue;}
		if (strnicmp(argc[x],"Level=",6)==0) {strcpy(Level,&argc[x][6]);   x++; continue;}
		x++; continue;						// Unknown parm
		}
return;
}
//
// Copy A string for p to s and move p forward
//
char * GetString( char *s, char *p, DWORD size ) 
{ 
    strncpy( s, p, size ); 
    return p + strlen(p) + 1; 
} 
//
// Peek into the registry and pull out the Message File Name for this Application
//
int GetAppMsgFile(char *Class,char *app,char *fname)
{
long rc;
HKEY hkey;
DWORD dwt=0,dwl=_MAX_PATH;
char key[MAX_PATH+1];
char hold[4096];
char *c,*p;
int x=0;

	sprintf(key,"%s\\%s\\%s",REG_KEY,Class,app);
	if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,key,0,KEY_READ,&hkey))!=ERROR_SUCCESS) return FALSE;
	RegQueryValueEx(hkey,"EventMessageFile",NULL,&dwt,hold,&dwl);
	RegCloseKey(hkey);
	c=strstr(hold,";"); p=hold;
	while(c) 
		{
		if(c) *c='\0';
		ExpandEnvironmentStrings(p,MsgFiles[x++],_MAX_PATH);
		c++; p=c;
		c=strstr(p,";");
		}
	if(p) ExpandEnvironmentStrings(p,MsgFiles[x],_MAX_PATH);
	NumOfFiles=x+1;
	strcpy(fname,MsgFiles[0]);
return TRUE;
}
//
// Peek into the registry and pull out the Parameter File Name for this Application
//
int GetParmMsgFile(char *Class,char *app,char *fname)
{
long rc;
HKEY hkey;
DWORD dwt=0,dwl=_MAX_PATH;
char key[MAX_PATH+1];
char hold[4096];

	sprintf(key,"%s\\%s\\%s",REG_KEY,Class,app);
	if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,key,0,KEY_READ,&hkey))!=ERROR_SUCCESS) return FALSE;
	RegQueryValueEx(hkey,"ParameterMessageFile",NULL,&dwt,hold,&dwl);
	RegCloseKey(hkey);
	ExpandEnvironmentStrings(hold,fname,_MAX_PATH);
return TRUE;
}

//
// Move strings to the Global Area for Format Message
//
int CopyStrings(char *p, int count) 
{ 
int x=0;

	if(count==0) return TRUE;
	if(!p) return FALSE;
	for(x=0;x<count;x++)
		{
		strncpy(Strings[x], p, MAX_STRING_LEN); 
		p=p + strlen(p) + 1;
		}	
return TRUE; 
} 
//
// Clear out Bogus Chars
//
int UpdateMessageText(char *Msg,int len)
{
int x;

	for(x=0;x<len;x++)
		{
		if( Msg[x]<' ' || Msg[x]==';') Msg[x]=' ';
		}
return x;
}
//
// Write data to Log
//
int WriteMsgFile(EVENTLOGRECORD *e)
{
char header[]="Machine;Class;AppName;Date;Time;EventType;EventCatagory;Message\n";
char buffer[4096];
char date[11];
char time[9];
char *type;
int num;
struct tm *evt;
time_t tme;

	if(!reccount) 
		{
		if(!Append) out=fopen(OutName,"w");						// Open up output File
		else out=fopen(OutName,"a+");							// Open for Append
		if(!out) exit(16);										// Get out of Town
		if(!Append) {
			num=fwrite(header,strlen(header),1,out);			// Write out the header
			if(!num) return 0;									// Was that OK
			else reccount++;									// Yepper
			}
		}
	tme=e->TimeWritten;											// Move to correct structure
	evt=localtime(&tme);										// Covert it please
	if(evt->tm_year<50) evt->tm_year+= 2000;					// Normalize year
	else evt->tm_year+=1900;
	evt->tm_mon++;												// Zero based
	sprintf(date,"%.2d/%.2d/%.4d",evt->tm_mon,evt->tm_mday,evt->tm_year);
	sprintf(time,"%.2d:%.2d:%.2d",evt->tm_hour,evt->tm_min,evt->tm_sec);
	type=SetEventType(e->EventType);
	sprintf(buffer,"%s;%s;%s;%s;%s;%s;%d;%s\n",Computer,Class,AppName,date,time,type,e->EventCategory,Msgbuf);
	num=fwrite(buffer,strlen(buffer),1,out);
	reccount++;
return num;
}
//
// Poof turn into English
//
char *SetEventType(WORD type)
{
	if(type==EVENTLOG_ERROR_TYPE)		return "Error";
	if(type==EVENTLOG_WARNING_TYPE)		return "Warning";
	if(type==EVENTLOG_INFORMATION_TYPE) return "Information";
	if(type==EVENTLOG_AUDIT_SUCCESS)	return "Audit Success";
	if(type==EVENTLOG_AUDIT_FAILURE)	return "Audit Failure";

}
//
// Check to see if level id OK
//
int LevelOk(char *Class,WORD level,char *requested)
{
	if(requested[0]=='I' || requested[0]=='i') return TRUE;
	if(requested[0]=='W' || requested[0]=='w') 
		{
		if( (stricmp(Class,CLASS_SEC)==0) && (level==EVENTLOG_AUDIT_FAILURE) ) return TRUE;
		if( (stricmp(Class,CLASS_SEC)!=0) && (level==EVENTLOG_WARNING_TYPE) ) return TRUE;
		if( (stricmp(Class,CLASS_SEC)!=0) && (level==EVENTLOG_ERROR_TYPE) ) return TRUE;
		return FALSE;
		}
	if(requested[0]=='E' || requested[0]=='e')
		{
		if( (stricmp(Class,CLASS_SEC)==0) && (level==EVENTLOG_AUDIT_FAILURE) ) return TRUE;
		if( (stricmp(Class,CLASS_SEC)!=0) && (level==EVENTLOG_ERROR_TYPE) ) return TRUE;
		return FALSE;
		}

return FALSE;
}
//
// Format a string and replace all the %% with parm information
//
int FormatParmMessage(char *Class,char *AppName,char *Message)
{
char *p,*s;
int length;
char fname[_MAX_PATH+1];
char msg[2048];
char local[4096];
HANDLE mhandle;

	GetParmMsgFile(Class,AppName,fname);					// Load in the file
	mhandle=LoadLibraryEx(fname,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
	if(!mhandle) {strcpy(Msgbuf,Message); return TRUE;}		// DLL Not found
	trim(Message);											// Get rid of extra stuff
	s=Message;													
	memset(local,0,sizeof(local));

	while( (p=strstr(s,"%"))!=NULL )
		{
		memset(msg,0,sizeof(msg));
		strncpy(local,s,(p-s));
		while (*p=='%') p++; 
		while( (isdigit(*p)) ) p++; 
		length=FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
				 FORMAT_MESSAGE_ARGUMENT_ARRAY | 60,  
				 mhandle,
				 atoi(p),
				 LANG_USER_DEFAULT,  
				 (LPTSTR) &msg, 
				 MAXMSGSIZE,		 
				 lpargs);									// Format this Message
		trim(msg);
		strcat(local,msg);									// Keep Building the message
		s=p;												// Now start from here
		}
	if(s) strcat(local,s);									// Complete Message
	if(local[0]>' ') strcpy(Msgbuf,local);					// Copy back over
	else strcpy(Msgbuf,Message);
return TRUE;
}
//
// Remove the Blamks
//
int trim(char *data)
{
int x=0;

	x=strlen(data);
	if(x<1) return 0;
	x--;
	while((data[x]<=' ') && x) {data[x]=0; x--;}
return x;
}
