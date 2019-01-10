#define MAXPARMS	15
#define ParmvSize	512
#define	MAXLVBASE	24
//
// defines used by RegUpd
//
#define DATABUFSIZE  8192
#define FILENAMEBUF  1024
#define REGOPER      24
#define REGKEY       2048
#define REGVALUENAME 1024
#define REGDATATYPE  32
#define REGHIVE      64
#define EDMSIZE		 8
#define REGDATA      DATABUFSIZE
#define OPERATION    "REGOPR"
#define KEYNAME      "REGKEY"
#define VALUE        "REGVALUE"
#define DATATYPE     "REGDATATYPE"
#define DATA         "REGDATA"  
#define HIVE         "REGHIVE"   
#define SECPREFIX    "SEC"    
#define EDMOBJ		 "OBJECT"
#define EDMVAR		 "VARIABLE"
#define EDMHEAP		 "HEAP"

//*****************************************************************************
// Definition for a Function Parm Block
//*****************************************************************************
typedef struct Lineent           
{
char	*line;										// Pointer to a line
int		num;										// Line number
int		type;										// Line type
int		level;										// Level ( "if/end" compare)
int		proclevel;									// Proc level
struct  Lineent *next;								// Next line
struct  Lineent *prev;								// Previous Line
} LENT,*PLENT;

//*****************************************************************************
// Definition for a script file in storage
//*****************************************************************************
typedef struct Script           
{		
char	sname[_MAX_PATH+1];							// Name of this script
char	sbase[_MAX_FNAME+1];						// Base name for variables
FILE	*sfile;										// File pointer for script
int		level;										// Call level for script
int		plevel;										// Proc level;
int		fatal;										// Fatal Error Please stop
struct  Lineent *lfirst;							// First line of script
struct  Lineent *lnext;								// Next line of script to execute
struct  Lineent *preturn;							// Next line after proc call
struct  Script *prev;								// Previous Script structure
struct	Script *next;								// Next Script structure 
} SCR,*PSCR;
//*****************************************************************************
// Definition for the parms of a call
//*****************************************************************************
typedef struct parmblk           
{
int		numberofparms;								// Number of parms the parser found
PSCR	curr_ps;									// Current script block
char	result[_MAX_PATH+1];						// Result from function
char	vartoset[_MAX_PATH+1];						// Result from function call goes to this var
char	*parms[MAXPARMS];							// Parms for call
} PARMBLK,*PPARMBLK;
char parmv[MAXPARMS][ParmvSize];					// Parms for input
/******************************************************/
/**     Defines for the routine build functions      **/
/******************************************************/
typedef struct 
{
char	*funcname;
void	(*function)(PPARMBLK pb);	
}FUNCDEF,*PFUNCDEF;
//*****************************************************************************
// Definition of a file Block 
//*****************************************************************************
typedef struct FileBlock           
{		
FILE	*sfile;										// File pointer 
char	type[8];									// Open as type
} FB,*PFB;
//*****************************************************************************
// Definition for a Variable Definition
//*****************************************************************************
typedef struct Varent           
{
struct  Varent *next;								// Next Variable
struct	Varent *ref;								// Reference for this var
int		len;										// length of variable
int		type;										// Variable type
char	*data;										// Data for Variable
char	name[64];									// NAME for Variable
} VENT,*PVENT;
typedef struct VentBase
{
struct Varent	*first;								// First Varentry
struct Varent	*last;								// Last Varentry
int				count;								// Number of entries
}VENTBASE,*PVENTBASE;