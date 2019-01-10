#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include "w32trace.h"
#define SEP ","
int dump(char *fn);
int Set_Data(char *vname,char *data,char *bufv,char *bufd);
void StartEXEC(char *name,int wait);
FILE *out;
FILE *in=NULL;

void main (int argn,char *argc[])
{
char cmd[1024];
char inf[_MAX_PATH+1];

	OpenLog("AMDIPCFG");

	strcpy(cmd,"cmd.exe /c ipconfig.exe /all 1>");
	strcpy(inf,W32LOGDIR); strcat(inf,"IPCONFIG.OUT");
	strcat(cmd,inf);

	StartEXEC(cmd,1);
	dump(inf);
return;
}
int dump(char *fn)
{
char buf[512];							// Input file Buffer
char bufv[2048];						// Variable Buffer
char bufd[2048];						// Data Buffer
char vbuf[64];							// Variable name buffer
int  adcnt=0;							// Adapter Count
char ofle[_MAX_PATH];


	strcpy(ofle,W32LOGDIR);				// Copy over the dir
	strcat(ofle,"IP.CSV");				// create the name
	out=fopen(ofle,"w");				// Open Output File
	in=fopen(fn,"r");					// Open File for read
	memset(bufv,0,sizeof(bufv));		// Clear it out
	memset(bufd,0,sizeof(bufd));		// Cleat it out
	while( (fgets(buf,512,in)) )		// Read ALl lines please
		{
		if(strstr(buf,"Host Name"))			
			{Set_Data("Host Name",strstr(buf,":"),bufv,bufd); continue;}
		if(strstr(buf,"DNS Servers"))		 
			{Set_Data("DNS Servers",strstr(buf,":"),bufv,bufd); continue;} 
		if(strstr(buf,"Node Type"))			 
			{Set_Data("Node Type",strstr(buf,":"),bufv,bufd); continue;} 
		if(strstr(buf,"NetBIOS ScopeId"))	
			{Set_Data("NetBios ScopeID",strstr(buf,":"),bufv,bufd); continue;} 
		if(strstr(buf,"IP Routing"))		 
			{Set_Data("IP Routing",strstr(buf,":"),bufv,bufd); continue;}
		if(strstr(buf,"WINS Proxy"))			
			{Set_Data("Wins Proxy",strstr(buf,":"),bufv,bufd); continue;}
		if(strstr(buf,"NetBIOS Resolution")) 
			{Set_Data("NetBIOS Resolution",strstr(buf,":"),bufv,bufd); continue;}
		if(strstr(buf,"Ethernet adapter")) {adcnt=adcnt+1; continue;}
		if(strstr(buf,"Description")) {
			sprintf(vbuf,"Adapter_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"Physical Address")) {
			sprintf(vbuf,"Address_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"DHCP Enabled")) {
			sprintf(vbuf,"DHCP_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"IP Address")) {
			sprintf(vbuf,"IPAddress_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"Subnet Mask")) {
			sprintf(vbuf,"Subnet_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"Default Gateway")) {
			sprintf(vbuf,"Gateway_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"DHCP Server")) {
			sprintf(vbuf,"DHCP_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"Primary WINS Server")) {
			sprintf(vbuf,"WINS_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"Secondary WINS Server")) {
			sprintf(vbuf,"SecWins%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"Lease Obtained")) {
			sprintf(vbuf,"Lease_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
		if(strstr(buf,"Lease Expires")) {
			sprintf(vbuf,"Expires_%.2d",adcnt);
			Set_Data(vbuf,strstr(buf,":"),bufv,bufd);
			continue;
			}
 		}
	strcat(bufv,"\n"); strcat(bufd,"\n");
	fwrite(bufv,strlen(bufv),1,out);
	fwrite(bufd,strlen(bufd),1,out);

return 1;
} 
int Set_Data(char *vname,char *data,char *bufv,char *bufd)
{

	data[strlen(data)-1]='\0';
	strcat(bufv,vname); trim(bufv); strcat(bufv,SEP);
	strcat(bufd,data+1); trim(bufd); strcat(bufd,SEP);
return 1;
}
void StartEXEC(char *name,int wait)
{
STARTUPINFO si = {0};
PROCESS_INFORMATION pi = {0};

      si.cb = sizeof(STARTUPINFO);
      si.lpReserved = NULL;
      si.lpReserved2 = NULL;
      si.cbReserved2 = 0;
      si.lpDesktop = NULL;
      si.dwFlags = 0;
	  si.wShowWindow=SW_SHOW;

      CreateProcess( NULL,
					 name,
                     NULL,
                     NULL,
                     FALSE,
					 CREATE_SEPARATE_WOW_VDM,
                     NULL,
                     NULL,
                     &si,
                     &pi );
	  if(!pi.hProcess)  
		{return;}
      if(wait) WaitForSingleObject( pi.hProcess, INFINITE );
	  CloseHandle(pi.hProcess);

return;
}

