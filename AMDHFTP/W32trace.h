/*************************************/
/** Log File Finctions			    **/
/*************************************/
extern int OpenLog(char *);
extern int CloseLog(void);
extern int WriteLog(char *);
extern int trim(char *);
char LogBuf[1024];
extern char LogDate[];
extern char LogTime[];
extern char W32LOGDIR[];
extern int Buildmsg(char *msg,void *p1,void *p2,void *p3,void *p4);
extern void lmsg(int type,const char *format,...);
extern void SetLogType(int type);
extern char *GetPathInfo(char *name);
extern int newlog;

/**************************************/
/** Define message types to use      **/
/**************************************/
#define MSG_ERR				1
#define MSG_INFO			10
#define MSG_DEBUG			25