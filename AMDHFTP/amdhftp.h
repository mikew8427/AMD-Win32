//
// Definitions for HoneyPot FTP server 
//
#define DEFAULT_PORT 21 
#define DEFAULT_PROTO SOCK_STREAM // TCP 
#define DLEN 8192
#define MLEN 1024
#define MAXPSWD 64
#define BUFLEN 256
#define ESIZE 32
char machname[MLEN]="D32Server";
char greeting[MLEN]="220 %s Microsoft FTP Server <Version 5.0>\n";
char pswd[MLEN]="331 Password required for %s\n";
char logon0[MLEN]="230- %s Logged on\n";
char logon1[MLEN]="     Unauthorized Use of this Site is Forbidden\n";
char logon2[MLEN]="230  Violators will be prosecuted\n";
char ok[MLEN]="200 %s command successful\n";
char ng[MLEN]="500 Syntax Error\n";
char datas[MLEN]="150 Opening ASCII mode data connection for /bin/ls.\n";
char tran[MLEN]="226 Transfer Complete\n";
char nlogon[MLEN]="530 User %s cannot log in\n";
char notfnd[MLEN]="550 %s : The system cannot find the file specified\n";
char type[MLEN]="200 Type set to %s\n";
char pwdr[MLEN]="257 \"%s\" is the current directory\n";
char startdir[MLEN]="c:\\";
extern char W32LOGDIR[];
char inifile[_MAX_PATH+1];
int SesCnt=0;
int MaxAttempts=0;
int MaxAlerts=0;
int IData=1;

//
//  Store Users Here
//
char users[MAXPSWD][32];
char passwords[MAXPSWD][32];


//
// Internal User and Password for shutdown
//
char user[MLEN];
char password[MLEN];
char ourdir[MLEN];
char ourpath[MLEN];
char events[MAXPSWD][ESIZE];					// Events to track
char evtcmd[MLEN];
//
// Network Port and Socket Definitions
//
unsigned short port=DEFAULT_PORT; 
int socket_type = DEFAULT_PROTO; 


//
// For tranaction data define a structure for Data record types + data
//
typedef struct session
{
SOCKET ms;				// Main session Socket
SOCKET ds;				// Data Socket
char user[256];			// User Name
char pswd[256];			// Password supplied
int type;				// Bin or ASCII
int	cmdcount;			// command count
int attempts;			// Login Attempts
int	alerts;				// Number of Alters for this Session
int login;				// Are we logged in
int	err;				// Number of errors
char path[_MAX_PATH+1];	// Current path
char buf[DLEN];			// Session Buffer
char parm[DLEN];		// Command Parm
unsigned short port;	// Data Port
DWORD ipaddr;			// Data Address
time_t stime;			// Login Time
int sesnum;				// This Session number
char ip[256];			// Caller IP
char currcmd[8];		// current command
} SESS,*PSES;

//
// Used during Session establishment to transfer information to the Session Thread
//
typedef struct sblk
{
SOCKET ms;
char ip[256];
int sessnum;
} SBLK,*PSBLK;

int GetData(PSES ses);
int DispCmd(PSES ses);
SOCKET OpenQoSSocket(INT iSocketType); 
int trim(char *data);
void Talk(PSBLK s);
int DoPort(PSES s);
int DoList(PSES s);
int DoCd(PSES s);
int DoUser(PSES ses);
void LoadConfigInformation();
void LoadUsers();
int ValidateUser(PSES s);
int DoType(PSES s);
int DoGet(PSES s);
int DoPut(PSES s);
int DoDelete(PSES s);
void StartHttp(char *fn);
void LoadEvents();
int CheckEvent(char *buf);
void Upper(char *buffer);
void lmsg2(int type,PSES s,const char *format,...);
int DoRecv(SOCKET s,PSES ps);
int DoNlst(PSES s);
void SendData(char *name,PSES s);














