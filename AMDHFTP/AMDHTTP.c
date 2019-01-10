#define WIN32_LEAN_AND_MEAN 
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h>
#include <direct.h>
#include <io.h>
#include <winsock2.h> 
#include "w32trace.h"
#define BLEN 4096

extern char inifile[];
extern char tempname[];
extern int SesCnt;
int Process(SOCKET s);
char Head[]="HTTP/1.1 200 OK\nDate: %sServer: Win2k Server\nContentType: text/html\n\n";

//
// Establish an HTTP response file
//
void StartHttp(char *fn)
{
char cPort[8];
unsigned short port;
SOCKET s,cs;
struct sockaddr_in my_addr, client_addr;
int clen,rc;


	GetPrivateProfileString("NETWORK","HTTPPORT","1932",cPort,7,inifile);
	port = atoi(cPort);

	s=socket(AF_INET,SOCK_STREAM,0);				// Build a socket for TCP
	my_addr.sin_port   =htons(port);						// Setup SOCKET for listen    
	my_addr.sin_family =AF_INET;
	my_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	rc=bind(s,(LPSOCKADDR)&my_addr,sizeof(my_addr));
	if(rc==SOCKET_ERROR) {lmsg(MSG_INFO,"Port [%d] Already in use",port); return; }
	rc=listen(s,1);
	if(rc==SOCKET_ERROR) {lmsg(MSG_INFO,"Listen error [%d] ",rc); return; }

	while(1)							
			{
			clen=sizeof(client_addr);
			cs=accept(s,(LPSOCKADDR)&client_addr,&clen);	/* Accept the session */
			Process(cs);
			}

}
//
// Process request
//
int Process(SOCKET s)
{
char buf[BLEN];
char ibuf[BLEN];
int rlen=64;
char machname[64];
FILE *in;
struct tm *gmt;
time_t ltime;
char tbuf[128];

	time(&ltime);
	gmt=gmtime(&ltime);
	sprintf(tbuf,"%s",asctime(gmt));
	tbuf[3]=',';
	GetComputerName(machname,&rlen);

	rlen= recv(s,buf,sizeof(buf),0 );			// Receive the request
	sprintf(buf,Head,tbuf);
	rlen=send(s,buf,strlen(buf),0);

	sprintf(buf,"<HTML><HEAD><BODY></HEAD><h3> AMDHFTP Deamon for Machine[%s] Total Sessions [%d]</h3><br><hr><br>\n",machname,SesCnt);
	rlen=send(s,buf,strlen(buf),0);
	in=fopen(tempname,"r");
	while(fgets(buf,BLEN,in)) {								// Start to read it in
		trim(buf);
		sprintf(ibuf," %s <br>",buf);
		rlen=send(s,ibuf,strlen(ibuf),0);
		}
	shutdown(s,2);											// Disable Send/Recv for socket
	closesocket(s);										// Now Close it

return rlen;
}