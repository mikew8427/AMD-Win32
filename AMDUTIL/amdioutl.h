//
// Definitions for the AMDIO Interface
//
#define MAXBLK	5

static int InitDone=0;


typedef struct AmdBlock {
int			inuse;
PAMDFILE	pv;
struct LineEntry *psort;
struct LineEntry *li;
} AMDBLOCK,*PAMDBLOCK;

AMDBLOCK	Master[MAXBLK];

