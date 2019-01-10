#define DATABUFSIZE		8192
#define FILENAMEBUF		1024
#define REGVALUENAME	1024
#define k32				1024*32
#define k1				1024
#define CRCLEN			8
#define IFSTM			1		// If statement
#define ENDSTM			2		// End Statement
#define ELSESTM			3		// Else Statement
#define EXESTM			4		// EXECUTE STATEMENT
#define EQUSTM			5		// Equate Statement
#define MAXVBASE		40
#define MAXVNAME		(_MAX_PATH*2)+1	
#define MAXBASE			_MAX_PATH+1
#define MAXVAR			_MAX_PATH+1
#define MAXLINE			1024
#define MAXROUTINENAME	24
//*****************************************************************************
HKEY ALLHKEY[5] = {HKEY_LOCAL_MACHINE , HKEY_CLASSES_ROOT , HKEY_CURRENT_USER , HKEY_USERS ,0};
char KEYROOT[5][45] = {"HKEY_LOCAL_MACHINE","HKEY_CLASSES_ROOT","HKEY_CURRENT_USER","HKEY_USERS",0};
//*****************************************************************************
// Global Definitions for the entire file
//*****************************************************************************
int		Setup(char *fname);							// Setup routine for Module
PSCR	BuildScript(char *fname);					// Build the Script structure
int		setline(char *buffer);						// format the line and remove blanks
int		FreeScript(PSCR ps);						// Free all storage for this entry
int		LoadScript(PSCR ps);						// read in the file
int		DumpScript(PSCR ps);						// DUmp lines to a log
int		ExecAtLevel(PSCR ps,int level,int plevel,PLENT l);	// Execute each line
int		LoadVbase(PSCR ps, char *name);				// Load up a Variable Base
int		FindVbase(PSLSTBASE vbase,char *name);		// Find a Vbase
int		Resolve(char *data);						// Resolve the line thanks
void	splitvar(char *full,char *base,char *var);	// Separate base name and var name
void	findvalue(char *full,char *base,char *var);	// Get value for a base/varname pair
void	getvalue(PSLSTBASE base,char *var,char *value); // Load data value found
int		setvalue(char *base,char *var,char *value);
PLENT	FindLabel(PSCR ps,char *name);				// Find a label
int		SaveVbase(PSCR ps, char *name);
int		GetNextLine(char *buffer,int size,int *l,PSCR ps);  // Build a complete line
int		FindLastString(char *data,char *what);		// Find the last string on a line
char *	LocalResolve(char *vname);					// Reset PB parm pointers
PVENT	VentSet(char *var,int len,char *value);		// Set a var Entry
PVENT	VentAdd(char *vname,char *size);			// Add an entry
PVENT	FindVent(char *vname);						// Using the name find the entry
PVENT	FindVentAdrress(char *ptr);					// Using the data address find the entry
PVENT	FindVentName(char *name);					// Find a VENT by name
int		VentFree(PVENT ent);						// Free a local var entry
int		VentDel(char *vname);						// Remove an entry from the current base
int		UpdateLocalVars(PSCR ps);					// After call update variables
void	SetupWorkDir(void);							// Setup for startup
void	SetArgs(void);

//*****************************************************************************
// Global Definitions for the entire file
//*****************************************************************************
char	filename[_MAX_PATH+1];						// Complete File Name
char	fname[_MAX_FNAME+1];						// File name holder
char	fext[_MAX_EXT+1];							// File name ext
char	fdrive[_MAX_DRIVE+1];						// File name Drive   
char	fdir[_MAX_DIR+1];							// File directory
PSCR	BASE=NULL;									// Base Script address
PSCR	CURRENT=NULL;								// Current Script Adrdess
int		ScriptLevel=0;								// Level of script
char	*args[25];									// Pointers to current args
char	argbuf[k1];									// Current argbuff 
int		argnum;										// Current number of args
int		ProcNumber;									// Number of procs in file
SLSTBASE Pbase;										// Proc name base
SLSTBASE Lbase;										// Label name base;
SLSTBASE Vbase[MAXVBASE];							// Set of 40 variable bases
int		VbaseNext=0;								// Next Open Vbase
PPARMBLK pb;										// Parm Block for parse
VENTBASE LVBase[MAXLVBASE];							// Local Variable Base
int		CurrentLVBase;								// Variable for Local LVBase