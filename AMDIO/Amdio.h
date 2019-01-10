/***********************************************
 Structure for in storage view of a VRM file
************************************************/
struct AMDFILE
{
struct AMDFILE *Next;		// Pointer to next pointer
unsigned long records;		// counter of records
unsigned long usecnt;		// User counter
unsigned long deletes;		// counter of writes
unsigned long inums;		// number of items
FILE *fptr;					// File pointer
char fname[MAX_PATH+1];		// File name opened
char *inames;				// Buffer with item Names
char *names[MAXITEMS];		// Pointers to the Names 
int  namesflg[MAXITEMS];	// Flags for each items
struct LineEntry *Fbase;	// Pointer to First Line Items
struct LineEntry *Lbase;	// Pointer to Last Line Item
struct LineItems Curr;		// Current line in use
} ;
typedef struct AMDFILE *PAMDFILE;
/***********************************************
 Structure VRM Dll
************************************************/
typedef struct AMDBase
{
char     BaseDir[_MAX_PATH+1];
PAMDFILE BaseAMD;
int		 AMDCount;
} AMDB, *PAMDB;

/* Easy defines to use */
#define NEWCELL         -1
#define MAXCBNUMBER     4095
#define MAXCELLNUMBER   2046
#define MAXFREENUMBER	1023
#define STARTITEMNUMBER 1
#define BaseCell		0
#define VRMEOF			-1
typedef int (*compfn) (const void *,const void*);

/* Prototypes for DLL Functions */
int        MsgtoScreen(char *);									// Write a message to the screen
int		   InitAMD();											// Basic setup for AMD
PAMDFILE   AllocAMD();											// Allocate the AMDFILE struct
PAMDFILE   AddAMD(char *fn);									// Add another AMD to the list
int		   RemoveAMD(PAMDFILE pr);								// Remove it from the list
void       AMDExit(void);										// Free All Storage
void	   CleanupAMDFile(PAMDFILE PV);							// Clean up the bist of the AMDFILE
int        FlushAMD(PAMDFILE pv);								// Flush the file 
int		   FlushItemNames(PAMDFILE pv,char *buffer);			// Write out Item Names
int		   SortAMD(PAMDFILE pv,char *item,struct LineEntry *sort[]);
int        __cdecl scomp(struct LineEntry *s1,struct LineEntry *s2);// Compare routine
int		   SortAMDA(PAMDFILE pv,char *item,struct LineEntry *sort[]);// Sort list Assending
int        __cdecl scompa(struct LineEntry *s1,struct LineEntry *s2);// Compare routine for assending
char	   *FindItem(struct LineEntry *le,int index);
int        CreateAMD(PAMDFILE pv);								// Create a AMD file
int        OpenAMD(PAMDFILE pv);								// Open a AMD file
int		   UnloadAMD(PAMDFILE pv);								// Read in the data records
int		   CloseAMD(PAMDFILE pv);								// Close a AMD file	
int		   RemoveBlanks(char *str);								// Remove Blanks from string
int		   BuildItemList(PAMDFILE pv,struct LineItems *li,struct LineEntry *le);// Breaks up ";" to "\0"
int		   BuildItemNames(char *buffer,PAMDFILE pv);			// Setup array of item Names
int        BuildLineEntry(struct LineEntry *le);				// Build a LIne entry
int		   GetIndexOfName(PAMDFILE pv,char *item);				// Returns Item Index
PAMDFILE   isAMD(char *fn);										// check if file name already open
int		   Select(PAMDFILE pv,int recnum);						// Get record #
int        Find(PAMDFILE pv,struct LineEntry *li,char *field,char *data,int len);
int		   FindSorted(PAMDFILE pv,struct LineEntry *list[],int index,char *field,char *data,int len);
int		   UpdateAMD(PAMDFILE pv,struct LineEntry *li,char *field,char *data);
int		   AddLiADM(PAMDFILE pv);								// Add a New Line Item
int		   DeleteLiADM(PAMDFILE pv,struct LineEntry *li);		// Delete a Line Item
int		   DeleteItemAMD(PAMDFILE pv,char *field);				// Deletes an item from the file
int		   AddItemAMD(PAMDFILE pv,char *field);					// Deletes an item from the file
int		   GetValueAMD(PAMDFILE pv,struct LineEntry *li,char *field,char *data,int length);
struct LineEntry *CopyAMD(PAMDFILE pv,struct LineEntry *from);	// Make a copy of a line item
int		   CopyItemsAMD(PAMDFILE pv1,PAMDFILE pv2);				// Make a copy of the fields
int        GetSortedItemAMD(PAMDFILE pv,struct LineEntry *from[],int num);
char       *FindItem(struct LineEntry *le,int index);			// Returns the pointer to an item
struct LineEntry * AllocLiAMD(PAMDFILE pv,int length);			// Allocate a line entry
struct LineEntry *GetSortedLiAMD(PAMDFILE pv,struct LineEntry *from[],int num);








