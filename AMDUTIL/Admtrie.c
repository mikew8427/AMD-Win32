#include "windows.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "admtrie.h"
#include "W32TRACE.h"
static PCRCT GLOBALTRIE;
static int NumberEntries=0;
static int NextEntry=0;
/*****************************************************************
Starting from root build a trie that represents that data. Use 
the "value" as the data if trie entry found.
*****************************************************************/
int addtrie(PCRCT lroot,char *data,int len,long value)
{
int x=0;
char c;
PCRCT entry;								/* Current entry in trie */

	entry=lroot;							/* Set local root pointer*/
	c=*data;
	while(c!=entry->item) 
		{
		if(entry->sib==NULL) {addchildren(entry,data,value); return 1;}
		entry=entry->sib;					/* just go to the next one  */
		}
	if(*(data+1)=='\0') 
		{
		lmsg(MSG_ERR,"Duplicate Value\n");
		return 0;
		}
	entry=entry->child;						/* Go down a level          */
	addtrie(entry,data+1,len,value);		/* Go do it again           */
return 1;
}
/*****************************************************************
Search the trie and see if this pattern is in there
*****************************************************************/
int findinroot(PCRCT lroot,char *data,long *value)
{
PCRCT next;
char c;
    
	next=lroot;
	c=*data;
	while(next!=NULL && c!=next->item ) next=next->sib;
	if(!next) return 0;
	if(next->child==NULL && *(data+1)=='\0') { *value=next->value; return 1;}
	if(*(data+1)=='\0') return 0;
	return(findinroot(next->child,data+1,value));
}
/*****************************************************************
Add in all the children under a sibling
*****************************************************************/
int addchildren(PCRCT lroot,char *data,long value)
{
char c;
PCRCT nentry;
PCRCT lentry;
int set=0;
c=*data; 

	while(c!='\0')
		{
		nentry=alloctrie();							/* Get storage of it		*/
		if(!nentry) return 0;						/* Not allocated			*/
		memset(nentry,0,sizeof(struct CRCtrie));	/* clear out entry			*/
		nentry->item=c;								/* load up this item		*/
		if(!set) lroot->sib=nentry;					/* First time set sibling	*/
		if(set) lentry->child=nentry;				/* Else add the children    */
		lentry=nentry;								/* Set last entry           */
		set++;										/* Set build count          */	
		c=*(data+set);								/* Next char entry          */
		}
	lentry->value=value;							/* Last entry get value     */
return 1;
}
/*****************************************************************
Add in all the children under a sibling
*****************************************************************/
int inittrie(int num)
{
	NumberEntries=num*1000;
	GLOBALTRIE = malloc( NumberEntries*(sizeof(struct CRCtrie)) );
	lmsg(MSG_ERR,"Allocate of [%d] trie entries complete\n",NumberEntries);
 return 1;
}
void * alloctrie()
{
	if(NextEntry>=NumberEntries) 
		return (malloc(sizeof(struct CRCtrie)));
	else return(GLOBALTRIE+NextEntry++);
return NULL;
}