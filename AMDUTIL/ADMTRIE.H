#ifndef TRIEDEFINED
/********************************************************/
/**        Trie structure for Trie routines            **/
/********************************************************/
typedef struct CRCtrie           
{
char   item;			  // char that is this trie
struct CRCtrie *sib;      // Sibling trie
struct CRCtrie *child;    // Child trie
long   value;			  // Data for this entry 
} CRCT,*PCRCT;

int addtrie     (PCRCT root,char *data,int len,long value);		/* Routine to build a trie     */
int addchildren (PCRCT root,char *data,long value);				/* Worker routine for addtrie  */			
int findinroot  (PCRCT root,char *data,long *value);			/* See if patter in this trie  */
int inittrie    (int num);										/* Allocate num of tries       */
void *			alloctrie();									/* Allocate these guyes        */
#endif
#define TRIEDEFINED 1