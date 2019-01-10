//
// Defines for Return codes
//
#define	GOOD		"0"
#define	BAD			"8"
#define	TRAGIC		"16"
#define MAXLINES	1000	

void	do_message(PPARMBLK pb);
void	do_system(PPARMBLK pb);
void	do_if(PPARMBLK pb);
void	do_end(PPARMBLK pb);
void	do_else(PPARMBLK pb);
void	do_call(PPARMBLK pb);
void	do_savevdf(PPARMBLK pb);
void	do_loadvdf(PPARMBLK pb);
void	do_setvalue(PPARMBLK pb);
void	do_goto(PPARMBLK pb);
void	do_add(PPARMBLK pb);
void	do_sub(PPARMBLK pb);
void	do_mult(PPARMBLK pb);
void	do_div(PPARMBLK pb);
void	do_link(PPARMBLK pb);
void	do_logmsg(PPARMBLK pb);
void	do_fileexits(PPARMBLK pb);
void	do_filegetdate(PPARMBLK pb);
void	do_filegettime(PPARMBLK pb);
void	do_filegetsize(PPARMBLK pb);
void	do_filegetattr(PPARMBLK pb);
void	do_filesetattr(PPARMBLK pb);
void	do_fileopen(PPARMBLK pb);
void	do_fileclose(PPARMBLK pb);
void	do_fileread(PPARMBLK pb);
void	do_filewrite(PPARMBLK pb);
void	do_stringtrim(PPARMBLK pb);
void	do_stringcopy(PPARMBLK pb);
void	do_stringsubstr(PPARMBLK pb);
void	do_stringfind(PPARMBLK pb);
void	do_stringlen(PPARMBLK pb);
void	do_stringcat(PPARMBLK pb);
void	do_localdefine(PPARMBLK pb);
void	do_localdrop(PPARMBLK pb);
void	do_while(PPARMBLK pb);
void	do_create32(PPARMBLK pb);
void	do_create16(PPARMBLK pb);
void	do_dispmsg(PPARMBLK pb);
void	do_regadd(PPARMBLK pb);
void	do_regupd(PPARMBLK pb);
void	do_regdelv(PPARMBLK pb);
void	do_regdelk(PPARMBLK pb);
void	do_regapp(PPARMBLK pb);
void	do_regpre(PPARMBLK pb);
void	do_regrem(PPARMBLK pb);
void	do_regget(PPARMBLK pb);
void	do_iniget(PPARMBLK pb);
void	do_iniset(PPARMBLK pb);
void	do_getsysdir(PPARMBLK pb);
void	do_getwindir(PPARMBLK pb);
void	do_mapdrive(PPARMBLK pb);
void	do_removedrive(PPARMBLK pb);
void	do_copyfile(PPARMBLK pb);
void	do_delay(PPARMBLK pb);
void	do_askuser(PPARMBLK pb);
void	do_fileremove(PPARMBLK pb);
void	do_createdir(PPARMBLK pb);
void	do_removedir(PPARMBLK pb);
void	do_computername(PPARMBLK pb);
void	do_StartAMDSession(PPARMBLK pb);
void	do_StopAMDSession(PPARMBLK pb);
void	do_SendAMDRequest(PPARMBLK pb);
void	do_filesetdatetime(PPARMBLK pb);
void	do_amdadd(PPARMBLK pb);
void	do_amdopen(PPARMBLK pb);
void	do_amdunload(PPARMBLK pb);
void	do_amdclose(PPARMBLK pb);
void	do_amdgetindexofname(PPARMBLK pb);
void	do_amdsort(PPARMBLK pb);
void	do_getsorteditem(PPARMBLK pb);
void	do_amdgetrecord(PPARMBLK pb);
void	do_amdgetvalue(PPARMBLK pb);
void	do_init(void);
void	do_amdremove(PPARMBLK pb);
void	do_amdsetvalue(PPARMBLK pb);
void	do_amddeleteitem(PPARMBLK pb);
void	do_amdadditem(PPARMBLK pb);
void	do_amdflush(PPARMBLK pb);
void	do_copycompress(PPARMBLK pb);
void	do_getcurrentdir(PPARMBLK pb);
void	do_setcurrentdir(PPARMBLK pb);
void	do_buildfilename(PPARMBLK pb);
void	do_freedisk(PPARMBLK pb);
void	do_totaldisk(PPARMBLK pb);
void	do_exetimeout(PPARMBLK pb);
void	do_setmaxlines(PPARMBLK pb);
void	do_getdate(PPARMBLK pb);
void	do_gettime(PPARMBLK pb);
void	do_getjulian(PPARMBLK pb);
void	do_getdateoffset(PPARMBLK pb);
void	do_filegetdatetime(PPARMBLK pb);

int		isroutine(char *routine,PPARMBLK pb);
int		scanparms(char *routine,PPARMBLK pb);
int		ParseRoutine(char *line,PPARMBLK pb);
int		ParseIF(char *line,PPARMBLK pb);
int		ParseElse(char *line,PPARMBLK pb);
int		ParseGoto(char *line,PPARMBLK pb);
int		ParseWhile(char *line,PPARMBLK pb);
int		findfirst(char *line,char *what);
int		findlast(char *line,char *what);
int		findnextparm(char *line,char *what);
int		parseparm(char *stm,char *l,char *r,char *c);
int		RemoveBlanks(char *data);
int		istrue(char *l,char *r,char *c);
void	ProcessDo(PPARMBLK pb);
PLENT   FindEnd(PLENT ps,int plevel,int level);	
PLENT   FindElse(PLENT ps,int plevel,int level);		
PLENT	FindProc(PSCR ps,char *name);
int		FindOpenFileBlock(void);
void	setresult(char *var,PPARMBLK pb);
WORD	SetShow(char *var);
int		GetNextBlk(void);
int		TestHandle(char *han);
int		LocateRecord(int hande,int recnum);
int		SetMasterLi(int handle,struct LineEntry *from[],int num);
int		isnum(char *c);
int		TestNum(char *l,char *r);


static int AMDSessionStarted=0;
static long ExeTimeout=-1;
static long LinesExecuted=0;
static long MaxLinesExe=0;								// Don;t care how many we execute
//
// Routines the this module needs to know about (support routines)
//
extern	int Resolve(char *data);
extern  int	ExecAtLevel(PSCR ps,int level,int plevel,PLENT l);	
extern	int	LoadVbase(PSCR ps, char *name);				// Load up a Variable Base
extern	int	SaveVbase(PSCR ps, char *name);
extern	int setvalue(char *base,char *var,char *value);
extern	void splitvar(char *full,char *base,char *var);	// Separate base name and var name
extern	PVENT VentAdd(char *vname,char *size);
extern	char * LocalResolve(char *vname);
extern  PVENT FindVent(char *vname);
extern	PVENT VentSet(char *var,int len,char *value);
extern	PVENT FindVentAdrress(char *ptr);
extern	int	UpdateLocalVars(PSCR ps);
extern	int	VentDel(char *vname);						// Remove an entry from the current base
extern	VENTBASE LVBase[MAXLVBASE];						// Local Variable Base
extern	int	CurrentLVBase;								// Variable for Local LVBase
extern	void DispMsg(void);
extern	int ProcessAction();							// Process Registry Calls
/************************************************/
/**	Global Variables used by RegUpd calls      **/
/************************************************/
extern int		CurrentSection;							// Used to build Section	
extern char		COper[REGOPER];							// Operation to perform   
extern char		CKey[REGKEY];							// Key name				
extern char		CValue[REGVALUENAME];					// Value name for Key     
extern char		CDataType[REGDATATYPE];					// Data type defined      
extern char		CData[REGDATA];							// Buffer for Data        
extern char		CHive[REGHIVE];							// Hive Name              
extern char		CObj[EDMSIZE+1];						// Edm Object Name		
extern char		CVar[EDMSIZE+1];						// EDM Var name			
extern char		CHeap[EDMSIZE+1];						// EDM HEap Number		
extern HKEY		CurrentHive;							// HKEY For hive          
extern char		DataBuffer[DATABUFSIZE]; 
