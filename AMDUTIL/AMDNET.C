#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include "nettcp.h"
#include "W32TRACE.h"
#include "crc.h"
#include "schglobal.h"
static int SESSION=0;								// Only One Session Allowed 
SOCKET s;
PINFBLK ib=NULL;								    // INfo block adrress         
WSADATA data;
SOCKADDR_IN server;
struct hostent *h;
int		DispTrans(PINFBLK ib);				// Dispatch the correct transaction
int		ExecCmd(PINFBLK ib);				// Execute The Command Please
int		TransEx(PINFBLK ib); 
int		DumpArea(char *data,int len);
void	FormatCmd(char *cmd,char *ext,char *p);
int		InitTrans(PINFBLK ib);
int		DumpArea(char *data,int len);
int		SendResponse(PINFBLK ib,int rc);
int		SendExec(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		ReqData(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		GetFileData(PINFBLK ib);
int		GetRecord(PINFBLK ib);
int		TransData(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		GetAMDValue(char *name);
int		StartExec(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		HoldData(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		GetAMDValue(char *name);
int		SendAMDRequest(char *type,char *fn,char *parm,char *loc);
int		SendCmd(PINFBLK ib,char *type,char *fn,char *p,char *l);// Send the Trans on the line
int		RecvTcp(PINFBLK ib);							// Recv from NETTCP.C
void	WriteTransBuffer(PINFBLK ib,char *type,char *data,int len);
void	Parms(char *in,char *out);
void	SetUpFileLocation(char *fn);



int StartTcpSession(char *addr)
{
int rc=0;
DWORD ipaddr;
char *type=NULL,*fn=NULL,*p=NULL,*l=NULL;				// parms passed to SendCmd
unsigned short port;
char cPort[32]="PORT";
	
	if(SESSION) return 0;							// Sorry Session Already Active
   	ib=malloc(sizeof(struct InfoBlk));				// Get memory for this block
	if(!InitIB(ib)) return 0;						// Allocate thr info block    
	gen_crc_table();								// Build crc table
	WSAStartup(WinSockVersion,&data);				// WinSock Startup            
	s=socket(AF_INET,SOCK_STREAM,0);				// Build a socket for TCP
	rc=setsockopt(ib->sock,IPPROTO_TCP,TCP_NODELAY,(const char *)&rc,sizeof(int));
	rc=SR_BUFF_SIZE+1024;
	rc=setsockopt(ib->sock,SOL_SOCKET,SO_RCVBUF,(const char *)&rc,sizeof(int));
	rc=SR_BUFF_SIZE+1024;
	rc=setsockopt(ib->sock,SOL_SOCKET,SO_SNDBUF,(const char *)&rc,sizeof(int));

	ipaddr=inet_addr(addr);						// Is it an IP address
	if(ipaddr!=INADDR_NONE) server.sin_addr.s_addr=ipaddr;
	else							 				// else its a name
		{
		h=gethostbyname(addr);	
		if(h==NULL) {lmsg(MSG_ERR,"Host [%s] not found",addr); return 0;}
		memcpy(&server.sin_addr,h->h_addr_list[0],h->h_length);
		}
	GetAMDValue(cPort);
	port=atoi(cPort);
	server.sin_port=htons(port);
	server.sin_family=AF_INET;
	if(connect(s,(LPSOCKADDR)&server,sizeof(server)))
		{
		rc=WSAGetLastError();
		lmsg(MSG_ERR,"Connect to host [%s] failed rc[%d]",addr,(void *)rc);
		StopClient(s);
		}
	ib->sock=s;									// Set into IB
	lmsg(MSG_INFO,"Connected to Host:[%s]",addr);
	SESSION=1;
return	1;

}
//
// Shutdown a client task
//
void StopClient(SOCKET s)
{
	shutdown(s,2);
	closesocket(s);									// Now Close it
	WSACleanup();					
	SESSION=0;
	lmsg(MSG_INFO,"TCP Session Terminated");
}
//
// Check The Command type and Exexcute what was asked for
//
int SendAMDRequest(char *type,char *fn,char *p,char *l)
{
int rc;
			
	lmsg(MSG_INFO,"Process TCP Request [%s] [%s] [%s] [%s]",type,fn,p,l);
	if(stricmp(type,"EXEC")==0)  {SendCmd(ib,type,fn,p,l);   }
	if(stricmp(type,"EXEO")==0)  {SendExec(ib,type,fn,p,l);  }
	if(stricmp(type,"GETF")==0)  {ReqData(ib,type,fn,p,l);   }
	if(stricmp(type,"DATA")==0)  {TransData(ib,type,fn,p,l); }
	if(stricmp(type,"START")==0) {StartExec(ib,type,fn,p,l); }
	if(stricmp(type,"STAGE")==0) {HoldData(ib,type,fn,p,l);  }
	StopDataOnTrans(ib);						// No More data for the transaction
	SendTrans(ib);								// Flush the buffer
	memset(ib->recbuf,0,16);					// Clear it out
	if(!RecvTDH(ib)) lmsg(MSG_INFO,"No Return value");
	rc=atoi(ib->recbuf);
return rc;
}
void Parms(char *in,char *out)
{
	if(!in) {out[0]='\0'; return;}
	if(in[0]!='\"') {strcpy(out,in); return;}
	if(in[strlen(in)-1]==34) in[strlen(in)-1]='\0';
	strcpy(out,&in[1]);
 return;
}
//
// SendCmd Sends the type along with the trans. Next record will be the file name,after that
// the data file for file. Types Are DATA or EXEC also send parms and location.
// * What ever you add as extra parts above and beyond p and l, always put them before
// * the data loop
//
int SendCmd(PINFBLK ib,char *type,char *fn,char *p,char *l)
{
int count=0;
FILE *in=NULL;											// Input file

	strcpy(ib->recbuf,type);							// Type is DATA or EXEC
	ib->recbuf_len=strlen(type);						// Set the length
	StartTrans(ib,(char)EXEC,0,TRUE);					// Set the transaction type to EXEC
	InitTrans(ib);										// Setup for Sends
	if(strlen(fn)) in=fopen(fn,"rb");					// Open for read the CMD File file
	if(!in) {
			lmsg(MSG_ERR,"Could not open File [%s]",fn);
			return 8;
			}		
	memset(ib->recbuf,0,REC_BUFF_SIZE);					// clear out the buffer
	memset(ib->srbuf,0,SR_BUFF_SIZE);					// Clear out send buffer
	WriteTransBuffer(ib,REC_FILE,fn,strlen(fn));		// Write Out file Name
	SendDataOnTrans(ib);								// Send it out
	if(p)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_PARM,p,strlen(p));		// Send ou the parms
		SendDataOnTrans(ib);							// Send it out
		}
	if(l)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_LOC,l,strlen(l));		// Send ou the Location
		SendDataOnTrans(ib);							// Send it out
		}
	if(!in) return 1;										// Already exits on remot computer
	while(ib->recbuf_len=fread(&ib->recbuf[1],1,(REC_BUFF_SIZE-1),in))
		{
		ib->recbuf[0]='D';								// Set data Type
		ib->recbuf_len++;								// Add in One
		SendDataOnTrans(ib);							// Send it out
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		}
return 1;
}
//
// TransData and SendCmd are basically the same. Rather that Muddle up the code with checks
// for if(type==data) I just copied the routine and changed the Transaction.
//
int TransData(PINFBLK ib,char *type,char *fn,char *p,char *l)
{
int count=0;
FILE *in=NULL;											// Input file

	memset(ib->srbuf,0,SR_BUFF_SIZE);					// Clear out send buffer
	strcpy(ib->recbuf,type);							// Type is DATA or EXEC
	ib->recbuf_len=strlen(type);						// Set the length
	StartTrans(ib,(char)TRDT,0,TRUE);					// Set the transaction type to EXEC
	InitTrans(ib);										// Setup for Sends
	if(strlen(fn)) in=fopen(fn,"rb");					// Open for read the CMD File file
	if(!in) {
			lmsg(MSG_ERR,"Could not open File [%s]",fn);
			return 0;
			}		
	memset(ib->recbuf,0,REC_BUFF_SIZE);					// clear out the buffer
	WriteTransBuffer(ib,REC_FILE,fn,strlen(fn));		// Write Out file Name
	SendDataOnTrans(ib);								// Send it out
	if(p)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_PARM,p,strlen(p));		// Send ou the parms
		SendDataOnTrans(ib);							// Send it out
		}
	if(l)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_LOC,l,strlen(l));		// Send ou the Location
		SendDataOnTrans(ib);							// Send it out
		}
	if(!in) return 1;										// Already exits on remot computer
	while(ib->recbuf_len=fread(&ib->recbuf[1],1,(REC_BUFF_SIZE-1),in))
		{
		ib->recbuf[0]='D';								// Set data Type
		ib->recbuf_len++;								// Add in One
		SendDataOnTrans(ib);							// Send it out
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		}
return 1;
}
void WriteTransBuffer(PINFBLK ib,char *type,char *data,int len)
{
	ib->recbuf_len=len+1;								// Set the length
	ib->recbuf[0]=type[0];								// Set File Name Please
	memcpy(&ib->recbuf[1],data,len);					// Copy it in
return;
}
//
// Only Execute what I pass you - No Data
//
int SendExec(PINFBLK ib,char *type,char *fn,char *p,char *l)
{
int count=0;

	strcpy(ib->recbuf,"EXEC");							// Type is DATA or EXEC
	ib->recbuf_len=4;									// Set the length
	StartTrans(ib,(char)EXEO,0,TRUE);					// Set the transaction type to EXEC
	InitTrans(ib);										// Setup for Sends
	memset(ib->recbuf,0,REC_BUFF_SIZE);					// clear out the buffer
	memset(ib->srbuf,0,SR_BUFF_SIZE);					// Clear out send buffer
	WriteTransBuffer(ib,REC_FILE,fn,strlen(fn));		// Write Out file Name
	SendDataOnTrans(ib);								// Send it out
	if(p)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_PARM,p,strlen(p));		// Send ou the parms
		SendDataOnTrans(ib);							// Send it out
		}
	if(l)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_LOC,l,strlen(l));		// Send ou the Location
		SendDataOnTrans(ib);							// Send it out
		}
return 1;
}
//
// Same as Exe Only but the other side will not wait for the process to end
//
int StartExec(PINFBLK ib,char *type,char *fn,char *p,char *l)
{
int count=0;

	strcpy(ib->recbuf,"EXEC");							// Type is DATA or EXEC
	ib->recbuf_len=4;									// Set the length
	StartTrans(ib,(char)STRT,0,TRUE);					// Set the transaction type to EXEC
	InitTrans(ib);										// Setup for Sends
	memset(ib->recbuf,0,REC_BUFF_SIZE);					// clear out the buffer
	memset(ib->srbuf,0,SR_BUFF_SIZE);					// Clear out send buffer
	WriteTransBuffer(ib,REC_FILE,fn,strlen(fn));		// Write Out file Name
	SendDataOnTrans(ib);								// Send it out
	if(p)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_PARM,p,strlen(p));		// Send ou the parms
		SendDataOnTrans(ib);							// Send it out
		}
	if(l)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_LOC,l,strlen(l));		// Send ou the Location
		SendDataOnTrans(ib);							// Send it out
		}
return 1;
}
//
// SendCmd Sends the type along with the trans. Next record will be the file name,after that
// the data file for file. Types Are DATA or EXEC also send parms and location.
// * What ever you add as extra parts above and beyond p and l, always put them before
// * the data loop
//
int ReqData(PINFBLK ib,char *type,char *fn,char *p,char *l)
{
int count=0;

	strcpy(ib->recbuf,type);							// Type is DATA or EXEC
	ib->recbuf_len=strlen(type);						// Set the length
	StartTrans(ib,(char)SNDF,0,TRUE);					// Set the transaction type to SNDF
	InitTrans(ib);										// Setup for Sends
	memset(ib->recbuf,0,REC_BUFF_SIZE);					// clear out the buffer
	memset(ib->srbuf,0,SR_BUFF_SIZE);					// Clear out send buffer
	WriteTransBuffer(ib,REC_FILE,fn,strlen(fn));		// Write Out file Name
	SendDataOnTrans(ib);								// Send it out
	if(p)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_PARM,p,strlen(p));		// Send ou the parms
		SendDataOnTrans(ib);							// Send it out
		}
	if(l)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_LOC,l,strlen(l));		// Send ou the Location
		SendDataOnTrans(ib);							// Send it out
		}
	StopDataOnTrans(ib);								// No More data for the transaction
	SendTrans(ib);										// Flush the buffer
	GetFileData(ib);									// Go Get the Data from client	
return 1;
}
int GetFileData(PINFBLK ib)
{
int count=0,dlen=0;
char type[36];
char f[_MAX_PATH+1];												// Full Name
char p[_MAX_PATH+1];												// Parms
char l[_MAX_PATH+1];												// Loction
FILE *out=NULL;
PBUFENT pb;
int rc;																// Buffer mapper

	memset(type,0,sizeof(type));									// Clear out Type Name
	memcpy(type,ib->recbuf,ib->recbuf_len);							// Save type away
	memset(f,0,sizeof(f));											// Clear it out
	memset(p,0,sizeof(p));
	memset(l,0,sizeof(l));
	pb=(PBUFENT)ib->recbuf;											// point to it
	while((rc=GetRecord(ib))>0)
		{
		if(ib->tdhtype==RSPNS) {
			printf("Reponse from file request was [%s]\n",ib->recbuf);// Could not get file
			if(strcmp(ib->recbuf,"004")==0) printf("File could not be opened or is not found\n");
			return FALSE;
			}
		dlen=ib->recbuf_len-1;										// Save the data length
		if(pb->type=='F') {
			memcpy(f,pb->data,dlen);								// Copy in the File Name
			}
		if(pb->type=='D')	{ 
			if(!out && strlen(f) ) {SetUpFileLocation(f); out=fopen(f,"wb");	}	// Open it Up
			if(out) {fwrite(pb->data,dlen,1,out); count++;}			// Write it out
			}
		}
	if(rc<0) return 16;												// Bad return
	if (out) {
		lmsg(MSG_INFO,"Number of records Written were[%d]",count);	// Dump out number of records	
		fclose(out);
		}
return 1;
}
int GetRecord(PINFBLK ib)
{
	memset(ib->recbuf,0,REC_BUFF_SIZE);								// Setup for record
	ib->recbuf_len=0;												// Set length to 0
	while(!ib->recbuf_len)
		{
		if(!RecvTDH(ib)) {ib->term_session=1; return -1;}			// Get entry off line
		if(ib->tdhdesc==ENDSECTION) return 0;						// Are we at the end
		if(ib->recbuf_len==0)		continue;						// Null Record (Start Trans)
		}
return 1;
}
int GetAMDValue(char *name)
{
int x=1;
DWORD dwt=0,dwl=_MAX_PATH;
HKEY hkey;
long rc=0;
char buf[_MAX_PATH+1];

	if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_KEY,0,KEY_READ,&hkey))!=ERROR_SUCCESS) 
		{
		strcpy(name,"");
		return 0;
		}
	else 
		{
		strcpy(buf,name); name[0]='\0';
		RegQueryValueEx(hkey,buf,NULL,&dwt,name,&dwl);
		RegCloseKey(hkey);
		}
return 1;
}
//
// Send Data and Hold to Reboot
//
int HoldData(PINFBLK ib,char *type,char *fn,char *p,char *l)
{
int count=0;
FILE *in=NULL;											// Input file

	strcpy(ib->recbuf,type);							// Type is DATA or EXEC
	ib->recbuf_len=strlen(type);						// Set the length
	StartTrans(ib,(char)LOCK,0,TRUE);					// Set the transaction type to EXEC
	InitTrans(ib);										// Setup for Sends
	if(strlen(fn)) in=fopen(fn,"rb");						// Open for read the CMD File file
	if(!in) {
			lmsg(MSG_ERR,"Could not open File [%s]",fn);
			return 8;
			}		
	memset(ib->recbuf,0,REC_BUFF_SIZE);					// clear out the buffer
	memset(ib->srbuf,0,SR_BUFF_SIZE);					// Clear out send buffer
	WriteTransBuffer(ib,REC_FILE,fn,strlen(fn));		// Write Out file Name
	SendDataOnTrans(ib);								// Send it out
	if(p)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_PARM,p,strlen(p));		// Send ou the parms
		SendDataOnTrans(ib);							// Send it out
		}
	if(l)
		{
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		WriteTransBuffer(ib,REC_LOC,l,strlen(l));		// Send ou the Location
		SendDataOnTrans(ib);							// Send it out
		}
	if(!in) return 1;										// Already exits on remot computer
	while(ib->recbuf_len=fread(&ib->recbuf[1],1,(REC_BUFF_SIZE-1),in))
		{
		ib->recbuf[0]='D';								// Set data Type
		ib->recbuf_len++;								// Add in One
		SendDataOnTrans(ib);							// Send it out
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		}
return 1;
}
void SetUpFileLocation(char *fn)
{
char buff2[_MAX_PATH+1];
char buffer[_MAX_PATH+1];
char *c,*s;

	memset(buffer,0,sizeof(buffer));
	strcpy(buff2,fn);
	s=buff2; c=s;
	while(c) 
		{
		c=strstr(c,"\\");											
		if(!c) break;												
		*(c)='\0';													
		strcat(buffer,s);											
		mkdir(buffer);												
		strcat(buffer,"\\");										
		c=c+1; s=c;													
		}
return;
}
