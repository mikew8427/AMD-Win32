/******************************************************************************\ 
* simples.c - Simple TCP/UDP server using Winsock 1.1 
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright 1996 - 1998 Microsoft Corporation. 
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation. 
*       See these sources for detailed information regarding the 
*       Microsoft samples programs. 
\******************************************************************************/ 

#define WIN32_LEAN_AND_MEAN 
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h>
#include <direct.h>
#include <io.h>
#include <winsock2.h> 
#include "amdhftp.h"
#include "w32trace.h"
#include "service.h"
extern char W32LOGDIR[];

  
int amdhftp(char *arg) {  

    char *interface= NULL; 
    int retval; 
    int fromlen; 
    struct sockaddr_in local, from; 
    WSADATA wsaData; 
    SOCKET listen_socket, msgsock; 
    int bOnce = 0; 
	HANDLE child;
	DWORD childid;


	OpenLog("AMDHFTP");
	strcpy(ourdir,W32LOGDIR);
	strcpy(ourpath,&ourdir[3]);
	if(ourpath[strlen(ourpath)-1] == '\\') ourpath[strlen(ourpath)-1] = '\0';

	sprintf(inifile,"%sAMDHFTP.INI",W32LOGDIR);

	LoadEvents();
	LoadConfigInformation();
	LoadUsers();

    if ((retval = WSAStartup(0x202,&wsaData)) != 0) { 
        fprintf(stderr,"WSAStartup failed with error %d\n",retval); 
        WSACleanup(); 
        return -1; 
    } 
     
    local.sin_family = AF_INET; 
    local.sin_addr.s_addr = (!interface)?INADDR_ANY:inet_addr(interface);  

    /*  
     * Port MUST be in Network Byte Order 
     */ 
    local.sin_port = htons(port); 

    listen_socket = socket(AF_INET,SOCK_STREAM,0); 

    if (listen_socket == INVALID_SOCKET){ 
        fprintf(stderr,"socket() failed with error %d\n",WSAGetLastError()); 
        WSACleanup(); 
        return -1; 
    } 
    // 
    // bind() associates a local address and port combination with the 
    // socket just created. This is most useful when the application is a  
    // server that has a well-known port that clients know about in advance. 
    // 

    if (bind(listen_socket,(struct sockaddr*)&local,sizeof(local) )  
        == SOCKET_ERROR) { 
        lmsg(MSG_ERR,"bind() failed with error [%d] Check that no other FTP Server is running on this machine",WSAGetLastError()); 
        WSACleanup(); 
        return -1; 
    } 

    // 
    // So far, everything we did was applicable to TCP as well as UDP. 
    // However, there are certain steps that do not work when the server is 
    // using UDP. 
    // 

    // We cannot listen() on a UDP socket. 

    if (socket_type != SOCK_DGRAM) { 
        if (listen(listen_socket,5) == SOCKET_ERROR) { 
            lmsg(MSG_ERR,"listen() failed with error [%d] Check for other FTP servers running on this machine",WSAGetLastError()); 
            WSACleanup(); 
            return -1; 
        } 
    } 
    lmsg(MSG_INFO,"%s: 'Listening' on port %d, protocol %s\n",SZAPPNAME,port,(socket_type == SOCK_STREAM)?"TCP":"UDP"); 
	//child=CreateThread(NULL,8192,(LPTHREAD_START_ROUTINE)StartHttp,(LPVOID)inifile,0,&childid);
	//CloseHandle(child);


    while(1) { 
		HANDLE child;
		DWORD childid;
		SBLK s;

        bOnce = 0; 
        fromlen =sizeof(from); 

        // 
        // accept() doesn't make sense on UDP, since we do not listen() 
        // 
        msgsock = accept(listen_socket,(struct sockaddr*)&from, &fromlen); 
        if (msgsock == INVALID_SOCKET) { 
                lmsg(MSG_INFO,"accept() error %d\n",WSAGetLastError()); 
				continue;
	            } 
		LoadConfigInformation();
		sprintf(s.ip,"%s",inet_ntoa(from.sin_addr));
		s.ms=msgsock;
		s.sessnum=++SesCnt;
        lmsg(MSG_INFO,"%d;%s;ACTION=ACCEPT",SesCnt,inet_ntoa(from.sin_addr),htons(from.sin_port)) ; 
		child=CreateThread(NULL,8192,(LPTHREAD_START_ROUTINE)Talk,(LPVOID)&s,0,&childid);
		CloseHandle(child);
	
	} //End of While loop

} 


//
// Session Thread executes here
//
void Talk(PSBLK s) 
{
PSES ses;
int retval; 
time_t now;
double total=0;

	ses=malloc(sizeof(SESS));
	memset(ses,0,sizeof(SESS));									// Clear out Session Block
	ses->ms=s->ms;												// save Away The socket
	strcpy(ses->ip,s->ip);										// Copy over caller IP
	ses->sesnum=s->sessnum;										// Copy over session Manager
	time(&ses->stime);											// Get time now

	strcpy(ses->path,startdir);									// Load up starting dir
	if(ses->path[strlen(ses->path)-1] != '\\') strcat(ses->path,"\\"); // make it valid
	sprintf(ses->buf,greeting,machname);						// Display Message
	retval = send(ses->ms,ses->buf,strlen(ses->buf),0);			// Send it out
	memset(ses->buf,0,DLEN);									// Clear it out
	retval = DoRecv(ses->ms, ses);
	if (strncmp(ses->buf, "OPTS", 4) == 0) {					// did we get options?
		sprintf(ses->buf, "Please enter UserID\r\n");						// Display Message
		retval = send(ses->ms, ses->buf, strlen(ses->buf), 0);			// Send it out
		memset(ses->buf, 0, DLEN);
		retval = DoRecv(ses->ms, ses);	
		}
	
	strncpy(ses->user,&ses->buf[5],255);
	trim(ses->user);											// Save User into Session block
	lmsg2(MSG_INFO,ses,"%d;%s;ACTION=USER(%s)",ses->sesnum,ses->ip,ses->user);
	memset(ses->buf,0,sizeof(ses->buf));							
	sprintf(ses->buf,pswd,ses->user);
	retval = send(ses->ms,ses->buf,strlen(ses->buf),0);
	memset(ses->buf,0,DLEN);
	retval = DoRecv(ses->ms,ses); 
	strncpy(ses->pswd,&ses->buf[5],255); trim(ses->pswd);			// Save password entered
	if(ValidateUser(ses) ) {
		sprintf(ses->buf,logon0,ses->user);							// Build Message
		retval = send(ses->ms,ses->buf,strlen(ses->buf),0);			// Send out logon complete
		retval = send(ses->ms,logon1,strlen(logon1),0);				// Send out logon complete
		retval = send(ses->ms,logon2,strlen(logon2),0);				// Send out logon complete
		ses->login=1;												// not logged in
		lmsg2(MSG_INFO,ses,"%d;%s;ACTION=LOGON(%s)",ses->sesnum,ses->ip,ses->user);
		}
	else {
		ses->login=0;												// not logged in
		lmsg2(MSG_INFO,ses,"%d;%s;ACTION=LOGON_FAIL",ses->sesnum,ses->ip,ses->user);
		sprintf(ses->buf,nlogon,ses->user);
		retval = send(ses->ms,ses->buf,strlen(ses->buf),0);			// Send out logon incomplete
		}
	ses->attempts++;												// Count the login Attempts
	while(GetData(ses) ) {
		DispCmd(ses);
		}
	shutdown(ses->ms,2);											// Disable Send/DoRecv for socket
	closesocket(ses->ms);										// Now Close it
	shutdown(ses->ds,2);											// Disable Send/DoRecv for socket
	closesocket(ses->ds);										// Now Close it
	time(&now);													// Get time now
	total=difftime(now,ses->stime);								// find the difference
	if( (strcmp(ses->user,user)==0) && (strcmp(ses->pswd,password)==0) ) {
		lmsg(MSG_INFO,"Terminated AMDHFTP by Internal password");
		ExitProcess(0);
		}

	lmsg2(MSG_INFO,ses,"%d;%s;ACTION=CMDCOUNT(%d)",ses->sesnum,ses->ip,ses->cmdcount);
	lmsg2(MSG_INFO,ses,"%d;%s;ACTION=ERRCOUNT(%d)",ses->sesnum,ses->ip,ses->err);
	lmsg2(MSG_INFO,ses,"%d;%s;ACTION=LOGON_ATTEMPTS(%d)",ses->sesnum,ses->ip,ses->attempts);
	lmsg2(MSG_INFO,ses,"%d;%s;ACTION=ALERTS(%d)",ses->sesnum,ses->ip,ses->alerts);
	lmsg2(MSG_INFO,ses,"%d;%s;ACTION=Timer(%f)",ses->sesnum,ses->ip,total);
	lmsg2(MSG_INFO,ses,"%d;%s;ACTION=TERMINATE",ses->sesnum,ses->ip);
	free(ses);

return;
}
//
// Get FTP command
//
int GetData(PSES ses) 
{
int len=0;

	memset(ses->buf,0,DLEN);
	len = DoRecv(ses->ms,ses);
	if(len<=0) {
		lmsg2(MSG_INFO,ses,"%d;%s;ACTION=CLIENTEXIT",ses->sesnum,ses->ip,ses->cmdcount);
		return 0;
		}
	trim(ses->buf);
	if(strnicmp(ses->buf,"QUIT",4) ==0) return 0;
return len;
}
//
// Split out command and parms. Then execute the command
//
int DispCmd(PSES ses) 
{
char *CMD;
char *PARM;
char *c=ses->buf;
char lbuf[DLEN];
int rc=0;

	memset(lbuf,0,DLEN);
	strcpy(lbuf,ses->buf);
	Upper(ses->buf);
	lmsg2(MSG_INFO,ses,"%d;%s;ACTION=RAWCMD(%s)",ses->sesnum,ses->ip,ses->buf);
	memset(ses->parm,0,DLEN);
	CMD=lbuf; c=CMD;							// Look at start of Buffer
	if(strlen(lbuf) > 4) c=strstr(lbuf," ");	// Find Blank of cmd
	else c=c+4;									// Else 4 char command ie QUIT
	if(!c) {									// Error 
		send(ses->ms,ng,strlen(ng),0);			// Send out the error
		return 0;								// return NG
		}
	*(c)='\0';									// swap a null for Blank
	PARM=++c;									// point at Parm
	strncpy(ses->parm,PARM,(DLEN-1));			// Save parm into buffer
	ses->cmdcount++;
	strncpy(ses->currcmd,CMD,4);

	if(stricmp("USER",CMD)==0) rc=DoUser(ses);	// User Please
	if(stricmp("PORT",CMD)==0) rc=DoPort(ses);	// Set up the session block for data
	//
	// If not Logged on You cant get past this point
	//
	if(!ses->login) {
		ses->err++;
		send(ses->ms,ng,strlen(ng),0);			// Send out the error
		return 0;								// return NG
		}

	if(stricmp("TYPE",CMD)==0) {
		DoType(ses);
		send(ses->ms,ses->buf,strlen(ses->buf),0);	// Send it on the wire
		return 1;
		}
	if(stricmp("RETR",CMD)==0 ) {
		DoGet(ses);
		send(ses->ms,ses->buf,strlen(ses->buf),0);	// Send it on the wire
		return 1;
		}
	if(stricmp("NLST",CMD)==0 ) {
		DoNlst(ses);
		send(ses->ms,ses->buf,strlen(ses->buf),0);	// Send it on the wire
		return 1;
		}

	if(stricmp("STOR",CMD)==0) {
		DoPut(ses);
		send(ses->ms,ses->buf,strlen(ses->buf),0);	// Send it on the wire
		return 1;
		}
	if( (stricmp("XPWD",CMD)==0) || (stricmp("PWD",CMD)==0) ) {
		sprintf(ses->buf,pwdr,ses->path);
		send(ses->ms,ses->buf,strlen(ses->buf),0);	// Send it on the wire
		return 1;
		}
	if(stricmp("XRMD",CMD)==0) {
		lmsg2(MSG_INFO,ses,"%d;%s;ACTION=RMDIR(%s)",ses->sesnum,ses->ip,ses->parm);
		rc=1;
		}
	if(stricmp("LIST",CMD)==0) rc=DoList(ses);
	if(stricmp("CWD",CMD)==0)  rc=DoCd(ses);
	if(stricmp("DELE",CMD)==0) rc=DoDelete(ses);


	if(rc) {
		memset(ses->buf,0,DLEN);				// Clear out Buffer
		sprintf(ses->buf,ok,CMD);				// Build OK Message						
		send(ses->ms,ses->buf,strlen(ses->buf),0);	// Send it on the wire
		}
	else {
		ses->err++;
		send(ses->ms,ng,strlen(ng),0);			// Send out the error
		return 0;								// return NG
		}
return rc;
}
//
// Process the TYPE Command
//
int DoType(PSES s)
{
	if(stricmp(s->parm,"A")==0) s->type=1;
	else s->type=0;
	sprintf(s->buf,type,(s->type == 0)?"I":"A");
return 1;
}
//
// Process the Port Command
//
int DoPort(PSES s)
{
unsigned short port,x;
char cPort[32]="";
char p1[4],p2[4],p3[4],p4[4],p5[4],p6[4];

	s->ds=socket(AF_INET,SOCK_STREAM,0);				// Build a socket for TCP
	sscanf(s->parm,"%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],",p1,p2,p3,p4,p5,p6);
	sprintf(cPort,"%s.%s.%s.%s",p1,p2,p3,p4);
	port=atoi(p5); x=atoi(p6);
	s->port = (port * 256) + x;
	s->ipaddr=inet_addr(cPort);						// Convert IP Address

return 1;
}
//
// Process the RETR/GET Command
//
int DoGet(PSES s)
{

char ndrive[MAX_PATH+1];
SOCKADDR_IN server;

	send(s->ms,datas,strlen(datas),0);						// Send it on the wire
//	s->ds=socket(AF_INET,SOCK_STREAM,0);					// Build a socket for TCP
	server.sin_addr.s_addr=s->ipaddr;
	server.sin_port=htons(s->port);
	server.sin_family=AF_INET;
	if(connect(s->ds,(LPSOCKADDR)&server,sizeof(server))) return 0;
	if(!s->ds) return 0;									// No port Command
	strcpy(ndrive,s->path); 
	if(ndrive[strlen(ndrive)-1] != '\\') strcat(ndrive,"\\");
	strcat(ndrive,s->parm);									// Set so we get all the files
	if(access(ndrive,0)==-1) sprintf(s->buf,notfnd,ndrive);	// No such file
	else { 
		SendData(ndrive,s);
		strcpy(s->buf,tran);								// Put in Transfer message
		}
	shutdown(s->ds,SD_BOTH);								// Disable Send/DoRecv for socket
	closesocket(s->ds);										// Now Close it
	s->ds=0;
return 1;
}

//
// Process the STOR/PUT Command
//
int DoPut(PSES s)
{
int len=0,total=0;
char ndrive[MAX_PATH+1];
SOCKADDR_IN server;

	send(s->ms,datas,strlen(datas),0);						// Send it on the wire
//	s->ds=socket(AF_INET,SOCK_STREAM,0);					// Build a socket for TCP
	server.sin_addr.s_addr=s->ipaddr;
	server.sin_port=htons(s->port);
	server.sin_family=AF_INET;
	if(connect(s->ds,(LPSOCKADDR)&server,sizeof(server))) return 0;
	if(!s->ds) return 0;									// No port Command
	strcpy(ndrive,s->path); 
	if(ndrive[strlen(ndrive)-1] != '\\') strcat(ndrive,"\\");
	strcat(ndrive,s->parm);									// Set so we get all the files
	lmsg2(MSG_INFO,s,"%d;%s;ACTION=STOR(%s)",s->sesnum,s->ip,ndrive);
	while ( (len=DoRecv(s->ds,s) ) ) {
		total=len+total;
		}
	strcpy(s->buf,tran);								// Put in Transfer message
	shutdown(s->ds,2);										// Disable Send/DoRecv for socket
	closesocket(s->ds);										// Now Close it
	s->ds=0;
return 1;
}

//
// Process the Port Command
//
int DoList(PSES s)
{
SYSTEMTIME st;
FILETIME ft1,ft2;
long size;
char dt[32];
HANDLE fhan=0;										// Handle from find first
char ndrive[MAX_PATH+1];
WIN32_FIND_DATA fdata;								// Data from find file
int rc;
SOCKADDR_IN server;

	memset(&fdata,0,sizeof(WIN32_FIND_DATA));
	send(s->ms,datas,strlen(datas),0);					// Send it on the wire
//	s->ds=socket(AF_INET,SOCK_STREAM,0);				// Build a socket for TCP
	server.sin_addr.s_addr=s->ipaddr;
	server.sin_port=htons(s->port);
	server.sin_family=AF_INET;
	if(connect(s->ds,(LPSOCKADDR)&server,sizeof(server))) {shutdown(s->ds,2);closesocket(s->ds);s->ds=0; return 0; }
	if(!s->ds) return 0;								// No port Command
	strcpy(ndrive,s->path); 
	if(ndrive[strlen(ndrive)-1] != '\\') strcat(ndrive,"\\");
	if(strlen(s->parm)==0) strcpy(s->parm,"*.*");
	strcat(ndrive,s->parm);									// Set so we get all the files
	if(!(fhan=FindFirstFile(ndrive,&fdata))) return 0;		// Get the handle
	while(1)
		{
		if(stricmp(fdata.cFileName,ourpath)==0) {if(!FindNextFile(fhan,&fdata)) break; continue;}
		if(fdata.cFileName[0]=='.') {if(!FindNextFile(fhan,&fdata)) break; continue;}
		if(!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
			{
			memcpy(&ft1,&fdata.ftLastWriteTime,sizeof(ft1));
			FileTimeToLocalFileTime(&ft1,&ft2);
			FileTimeToSystemTime(&ft2,&st);
			sprintf(dt,"%.2d-%.2d-%.2d %.2d:%.2d",st.wMonth,st.wDay,st.wYear,st.wHour,st.wMinute,st.wSecond);
			size=fdata.nFileSizeHigh+fdata.nFileSizeLow;
			sprintf(s->buf,"%s \t%8d %s\n",dt,size,fdata.cFileName);
			if(fdata.cFileName[0]>'\0') rc=send(s->ds,s->buf,strlen(s->buf),0);	// Send it on the wire
			}
		else {
			memcpy(&ft1,&fdata.ftLastWriteTime,sizeof(ft1));
			FileTimeToLocalFileTime(&ft1,&ft2);
			FileTimeToSystemTime(&ft2,&st);
			sprintf(dt,"%.2d-%.2d-%.2d %.2d:%.2d",st.wMonth,st.wDay,st.wYear,st.wHour,st.wMinute,st.wSecond);
			size=fdata.nFileSizeHigh+fdata.nFileSizeLow;

			sprintf(s->buf,"%s  <DIR>\t\t %s\n",dt,fdata.cFileName);
			rc=send(s->ds,s->buf,strlen(s->buf),0);	// Send it on the wire
			}
		if(!FindNextFile(fhan,&fdata)) break;
		}
send(s->ms,tran,strlen(tran),0);					// Send it on the wire
shutdown(s->ds,2);											// Disable Send/DoRecv for socket
closesocket(s->ds);										// Now Close it
s->ds=0;
FindClose(fhan);
return 1;
}
//
// Process the NLST Command
//
int DoNlst(PSES s)
{
HANDLE fhan=0;										// Handle from find first
char ndrive[MAX_PATH+1];
WIN32_FIND_DATA fdata;								// Data from find file
int rc;
SOCKADDR_IN server;

	memset(&fdata,0,sizeof(WIN32_FIND_DATA));
	send(s->ms,datas,strlen(datas),0);					// Send it on the wire
//	s->ds=socket(AF_INET,SOCK_STREAM,0);				// Build a socket for TCP
	server.sin_addr.s_addr=s->ipaddr;
	server.sin_port=htons(s->port);
	server.sin_family=AF_INET;
	if(connect(s->ds,(LPSOCKADDR)&server,sizeof(server))) return 0;
	if(!s->ds) return 0;								// No port Command
	strcpy(ndrive,s->path); 
	if(ndrive[strlen(ndrive)-1] != '\\') strcat(ndrive,"\\");
	if(strlen(s->parm)==0) strcpy(s->parm,"*.*");
	strcat(ndrive,s->parm);									// Set so we get all the files
	if(!(fhan=FindFirstFile(ndrive,&fdata))) return 0;		// Get the handle
	while(1)
		{
		if(stricmp(fdata.cFileName,ourpath)==0) {if(!FindNextFile(fhan,&fdata)) break; continue;}
		if(fdata.cFileName[0]=='.') {if(!FindNextFile(fhan,&fdata)) break; continue;}
		if(!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
			{
			sprintf(s->buf,"%s\n",fdata.cFileName);
			if(fdata.cFileName[0]>'\0') rc=send(s->ds,s->buf,strlen(s->buf),0);	// Send it on the wire
			}
		if(!FindNextFile(fhan,&fdata)) break;
		}
send(s->ms,tran,strlen(tran),0);					// Send it on the wire
shutdown(s->ds,2);											// Disable Send/DoRecv for socket
closesocket(s->ds);										// Now Close it
s->ds=0;
FindClose(fhan);
return 1;
}
//
// Send out Binary zeros as data
//
void SendData(char *name,PSES s)
{
int len;
char buf[DLEN];
long size;
HANDLE fh;
DWORD sl,sh;

	fh=CreateFile(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	sl=GetFileSize(fh,&sh);
	size=sl+sh;
	memset(buf,0,DLEN);
	while(size) {
		if(size > DLEN) { len=DLEN; size=size-DLEN; }
		else { len=size; size=0;}
		send(s->ds,buf,len,0);
		}
	CloseHandle(fh);
return;
}
//
// Change directory
//
int DoCd(PSES s)
{
char buf[DLEN];
int len=DLEN;
int x=0;

	if(s->parm[0]=='/') {									// If a Broswer talks to use it puts / as the 1st char
		strcpy(s->parm,&s->parm[1]);						// We will strip out the first one and replace all others
		while(s->parm[x]> '\0')								// with a \. This way we can do the CD in the dir
			{ 
			if(s->parm[x]== '/') { s->parm[x]='\\'; continue;} 
			else x++;
			}
		s->parm[strlen(s->parm)-1]='\0';
		strcpy(buf,s->parm);								// We are finished with the editing
		}
	else sprintf(buf,"%s%s",s->path,s->parm);				// From an FTP Client Just copy it in
	//
	// Protect ourselves. Hide our directory from the world
	//
	if(strnicmp(buf,ourdir,strlen(buf)) == 0) return 0;	// no can do charlie
	if(SetCurrentDirectory(buf)) {
		GetCurrentDirectory(len,s->path);
		if(s->path[strlen(s->path)-1] != '\\') strcat(s->path,"\\"); // make it valid
		return 1;		
		}

return 0;
}
//
// Process Delete Command
//
int DoDelete(PSES s)
{
char ndrive[_MAX_PATH+1];

	strcpy(ndrive,s->path); 
	if(ndrive[strlen(ndrive)-1] != '\\') strcat(ndrive,"\\");
	strcat(ndrive,s->parm);									// Set so we get all the files
	lmsg2(MSG_INFO,s,"%d;%s;ACTION=DELE(%s)",s->sesnum,s->ip,ndrive);

return 1;
}
//
// Change Active User
//
int DoUser(PSES ses)  
{
int retval; 

	strncpy(ses->user,ses->parm,255);
	trim(ses->user);												// Save User into Session block
	memset(ses->buf,0,sizeof(ses->buf));							
	sprintf(ses->buf,pswd,ses->user);
	retval = send(ses->ms,ses->buf,strlen(ses->buf),0);
	memset(ses->buf,0,MLEN);
	retval = DoRecv(ses->ms,ses); 
	strncpy(ses->pswd,&ses->buf[5],255); trim(ses->pswd);				// Save password entered
	if(ValidateUser(ses) ) {
		sprintf(ses->buf,logon0,ses->user);							// Build Message
		retval = send(ses->ms,ses->buf,strlen(ses->buf),0);			// Send out logon complete
		retval = send(ses->ms,logon1,strlen(logon1),0);				// Send out logon complete
		retval = send(ses->ms,logon2,strlen(logon2),0);				// Send out logon complete
		ses->login=1;												// not logged in
		lmsg2(MSG_INFO,ses,"%d;%s;ACTION=LOGON(%s)",ses->sesnum,ses->ip,ses->user);
		}
	else {
		ses->login=0;												// not logged in
		lmsg2(MSG_INFO,ses,"%d;%s;ACTION=LOGON_FAIL",ses->sesnum,ses->ip,ses->user);
		sprintf(ses->buf,nlogon,ses->user);
		retval = send(ses->ms,ses->buf,strlen(ses->buf),0);			// Send out logon incomplete
		}
	ses->attempts++;												// Count the login Attempts
	if(MaxAttempts==ses->attempts) {
		lmsg2(MSG_INFO,ses,"%d;%s;ACTION=LOGON_LIMIT",ses->sesnum,ses->ip,ses->user);
		}
	
return 1;
}

//
// Read The INI File for Custom Messages
//
void LoadConfigInformation()
{
char inifn[_MAX_PATH+1];
char cPort[8];
int len=MLEN;
char hold[MLEN];

	GetComputerName(machname,&len);

	sprintf(inifn,"%sAMDHFTP.INI",W32LOGDIR);
	GetPrivateProfileString("MESSAGES","GREETING","220 %s Microsoft FTP Server <Version 5.0>",greeting,MLEN-1,inifn);
	strcat(greeting,"\n");
	memset(logon2,5,' ');
	GetPrivateProfileString("MESSAGES","LOGON0","230- %s Logged on",logon0,MLEN-1,inifn);
	GetPrivateProfileString("MESSAGES","LOGON1","Unauthorized Use of this Site is Forbidden",&logon1[5],MLEN-1,inifn);
	GetPrivateProfileString("MESSAGES","LOGON2","230  Violators will be prosecuted",logon2,MLEN-1,inifn);
	strcat(logon0,"\n"); strcat(logon1,"\n"); strcat(logon2,"\n");
	GetPrivateProfileString("MESSAGES","DATAS","150 Opening data connection",datas,MLEN-1,inifn);
	strcat(datas,"\n");
	GetPrivateProfileString("MESSAGES","OK","200 %s command successful",ok,MLEN-1,inifn);
	strcat(ok,"\n");
	GetPrivateProfileString("MESSAGES","NG","500 Syntax Error",ng,MLEN-1,inifn);
	strcat(ng,"\n");
	GetPrivateProfileString("MESSAGES","NLOGON","530 User %s cannot Log in",nlogon,MLEN-1,inifn);
	strcat(nlogon,"\n");
	GetPrivateProfileString("MESSAGES","NOTFND","550 %s : The system cannot find the file specified",notfnd,MLEN-1,inifn);
	strcat(notfnd,"\n");
	GetPrivateProfileString("MESSAGES","TYPE","200 Type set to %s",type,MLEN-1,inifn);
	strcat(type,"\n");

	GetPrivateProfileString("NETWORK","PORT","21",cPort,7,inifn);
	port = atoi(cPort);
	GetPrivateProfileString("NETWORK","STARTDIR","C:\\",startdir,MLEN-1,inifn);
	if(startdir[strlen(startdir)-1] != '\\') strcat(startdir,"\\"); // make it valid

	GetPrivateProfileString("INTERNAL","USER","Special",user,MLEN-1,inifn);
	strcpy(users[0],user);
	GetPrivateProfileString("INTERNAL","PASSWORD","Special",password,MLEN-1,inifn);
	strcpy(passwords[0],password);
	GetPrivateProfileString("INTERNAL","InnocuousData","0",hold,MLEN-1,inifn);
	IData=atoi(hold);

return;
}
//
// Load up the Users/Passwords for this session
//
void LoadUsers()
{
int x;
char kn[8];
char buffer[BUFLEN];
char inifn[_MAX_PATH+1];
char *c;

	sprintf(inifn,"%sAMDHFTP.INI",W32LOGDIR);

	for(x=1;x<MAXPSWD; x++) {
		sprintf(kn,"USER%d",x);
		GetPrivateProfileString("USERS",kn,"",buffer,BUFLEN-1,inifn);
		if(buffer[0]=='\0') break;										// Nothing left to do
		c=strstr(buffer,";");
		if(!c) break;
		*(c)='\0'; c++;
		strcpy(users[x],buffer);
		strcpy(passwords[x],c);
		}

}
//
// Load up the Events for this session
//
void LoadEvents()
{
int x;
char kn[12];
char inifn[_MAX_PATH+1];
char buf[MLEN];

	sprintf(inifn,"%sAMDHFTP.INI",W32LOGDIR);
	GetPrivateProfileString("ALERT","CMD","",evtcmd,MLEN-1,inifn);			// get the command
	GetPrivateProfileString("ALERT","MAXALERTS","10",buf,MLEN-1,inifn);		// get the Session Alter Limit
	MaxAlerts=atoi(buf);
	if(MaxAlerts==0) MaxAlerts=10;											// Set default if needed
	GetPrivateProfileString("ALERT","LogonAttempts","8",buf,MLEN-1,inifn);		// get the Session Alter Limit
	MaxAttempts=atoi(buf);
	if(MaxAttempts==0) MaxAttempts=8;
	memset(events[0],0,ESIZE);
	for(x=0;x<MAXPSWD; x++) {
		sprintf(kn,"EVENT%d",x);
		GetPrivateProfileString("ALERT",kn,"",events[x],ESIZE-1,inifn);
		}

}
//
// Pass this routine the Event message and it will determine if this gets a trigger
//
int CheckEvent(char *buf)
{
int x=0;
char cmd[MLEN];
char *c=NULL;

	while(events[x][0]>' ') {
		if( (c=strstr(buf,events[x])) ) {
			sprintf(cmd,evtcmd, buf);
			system(cmd);
			return 1;
			}
		x++;
		if(x==MAXPSWD) break;
		}
return 0;
}
//
// Check user and passwords typed in
//
int ValidateUser(PSES s)
{
int x;
	for(x=0; x<MAXPSWD; x++) {
		if( (stricmp(users[x],s->user)==0) && (stricmp(passwords[x],s->pswd)==0) ) return 1;
		}
return 0;
}
void Upper(char *buffer)
{
int x=0;
	while(buffer[x]>'\0') {
		if(buffer[x] >= 'a' && buffer[x]<='z'  ) buffer[x]=_toupper(buffer[x]);
		x++;
		}
}

//
// Build a message from a variable list of args
//
void lmsg2(int type,PSES s,const char *format,...)
{
va_list args;
char buf[DLEN];
char msgbuf[DLEN];

	if(s) {
		if(s->alerts > MaxAlerts) return;
		}
	memset(buf,0,DLEN);
	memset(msgbuf,0,DLEN);
	va_start(args,format);			// Start the args
	vsprintf(buf,format,args);		// Format the buffer
	va_end(args);					// End args
	if( (CheckEvent(buf)) ) s->alerts++;
	lmsg(type,"%s",buf);			// Send it to the Logger
return;
}
//
// Wrapper for receive
//
int DoRecv(SOCKET s,PSES ps)
{
int retval;
int wserr;

	WSASetLastError(0);
	retval = recv(s,ps->buf,DLEN,0);
	wserr=WSAGetLastError();
	if( wserr==WSAEMSGSIZE )
		lmsg2(MSG_INFO,ps,"%d;%s;ACTION=BUFFEROVERFLOW(%d)",ps->sesnum,ps->ip,retval);

return retval;
}