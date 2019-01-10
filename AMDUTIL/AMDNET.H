#include "nettcp.h"
extern int SESSION;								// Only One Session Allowed 
extern SOCKET s;
extern PINFBLK ib;								    // INfo block adrress         
extern WSADATA data;
extern SOCKADDR_IN server;
int DumpArea(char *c,int len);
int SendCmd(PINFBLK ib,char *type,char *fn,char *p,char *l);// Send the Trans on the line
int	RecvTcp(PINFBLK ib);							// Recv from NETTCP.C
void WriteTransBuffer(PINFBLK ib,char *type,char *data,int len);
int StartTcpSession(char *addr);
int GetAMDValue(char *name);
void StopClient(SOCKET s);
int SendAMDRequest(char *type,char *fn,char *parm,char *loc);
