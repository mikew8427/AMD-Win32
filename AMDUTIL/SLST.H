#define ListName		64
//*****************************************************************************
// Definition for Single link list entry 
//*****************************************************************************
typedef struct Slst           
{
struct	Slst *next;									// Next list entry
void	*entry;										// Data Item 1 
void	*entry2;									// Data Item 2
void	*entry3;									// Data Item 3
} SLST,*PSLST;
//*****************************************************************************
// Definition for Base entry for link list
//*****************************************************************************
typedef struct SlstBase           
{
struct	Slst *first;								// First list entry
struct	Slst *last;									// Last entry
int		init;										// Init of list
int		entries;									// Number of entries
char    name[64];									// 64 byte name of list
} SLSTBASE,*PSLSTBASE;

int InitLstBase(PSLSTBASE base,char *name);			// Make base ready to start
PSLST AllocEnt(void);								// Get an entry	
int AddSlst(PSLSTBASE base,PSLST ent);				// Add the entry
int FreeSlst(PSLSTBASE base);						// Free all entries