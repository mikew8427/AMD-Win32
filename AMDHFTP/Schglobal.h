//
// Used for Global Defines for the Schedular
//
#define MAXBUF			32768
#define MAXITEMS		256
#define DEFAULTSEP		";"
#define MAXSTREAM		50
#define MAXNAME			256
#define MAXSEQ			24
#define MAXTRANS		9
#define RCLENGTH		4
#define REC_FILE		"F"
#define REC_PARM		"P"
#define REC_DATA		"D"
#define REC_LOC			"L"
#define REG_KEY			"Software\\AMD"
#define REG_AMDLIB		"AMDLIB"
#define REG_VAR			"VARIABLES"
#define REG_DATA		"DATA"
#define REG_TIMEO		"TIMEOUT"
#define REG_ERROR		"ERRORPGM"
#define REG_CFG			"CFGOK"
#define REG_SCHERR		"SCHERROR"
#define REG_PORT		"PORT"
#define REG_MACH		"Machine"
#define REG_ISERV		"INETSERV"
#define REG_IINT		"INTERVAL"


//
// Gloabl TCP/IP defines
//
#define WinSockVersion MAKEWORD(1,1)
#define DEFAULTHOST "AMDHOST"
#define PortNumber 5000
//
// Define Transaction Codes Here
//
#define EXEC			0x80			// Transfer and Execute 
#define EXEO			0x81			// Execute Only no transfer
#define	SNDF			0x83			// Send A file Back to requestor
#define TRDT			0x84			// Transfer a file to a Server
#define STRT			0x85			// Start a process
#define LOCK			0x86			// Transfer a file for reboot
#define STOP			0x87			// Stop a process
#define REMV			0x88			// Remove a file
#define STGD			0x89			// Stage a delete for reboot
#define DELD			0x8a			// Remove all files from a directory
#define GDIR			0x8b			// Group Dir list
#define PDIR			0x8c			// Project Dir List
#define PWDU			0x8d			// MS Windows Password Reset

//
// Define Directory Scan Types
//
#define SCANG			"GROUP"
#define SCANP			"PROJECT"
//
// Define MUXTEX names here for File Control Access
//
#define MUXNAME			"AMDMON_STATS_FILE"
#define MUXSCHFILE		"AMDMON_SCH_FILE_"		// Append the File Name onto this string
#define	MUXEXEC			"AMDMON_EXEC_FILE_"		// Append transaction file name
//
//structure for file unloads
//
struct LineEntry
{
struct LineEntry	*next;						// Next line entry
char				*data;						// Ponter to line data
int					length;						// full length of the data
int					status;						// Status of entry
};
//
// For Each SCH type file (; delimeter) Use a LineItems Struct to maintain in storage list
//
struct LineItems
{
struct LineItems	*next;						// Next Line Item Structure
int					length;						// Length of buffer
char				*buffer;					// Data Buffer
char				*items[MAXITEMS];			// Pointers into data buffer
} ;
//
// For tranaction data define a structure for Data record types + data
//
typedef struct bufentry
{
char type;
char data[];
} BUFENT,*PBUFENT;
//
// Machine List structure are the selected machines for this schedule
//
struct MachList
{
struct	MachList *next;					// Next Line Item Structure
char	Name	[MAXNAME];				// MAchine Name
char	Hname	[MAXNAME];				// Host name
char	scripts	[MAXSTREAM][MAXNAME];	// Up to 50 unique steps
char	seq		[MAXSTREAM][MAXSEQ];	// What to execute first
char	data	[MAXSTREAM][5];			// Data Type
char	rc		[MAXSTREAM][RCLENGTH];	// Return Code for Section
} ;
//
// Used by the dispatchers - It is "THE" entry for the Schedules Execution
//
typedef struct SCH
{
char	SchName[MAXNAME];				// Schedue Name
char	Stream[MAXNAME];				// Stream Name
int		MaxToDo;						// Number of Machines to process
int		ComplOk;						// Number of Successful Completion
int		CompErr;						// Number of Error Completions
char	Transid[MAXTRANS];				// Transid for this Schedule
struct	MachList *first;				// Pointer to the Machine List
} SCHENT,*PSCHENT;
