#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "slst.h"

//
// Make sure we start off OK
//
int InitLstBase(PSLSTBASE base,char *name)
{
	if(!base) return 0;
	base->first=NULL;
	base->last=NULL;
	base->init=1;
	base->entries=0;
	if(strlen(name)<ListName) strcpy(base->name,name);
	else strncpy(base->name,name,(ListName-1));
return 1;
}
//
// Add an entry to the end of the list
//
int AddSlst(PSLSTBASE base,PSLST ent)
{
PSLST hold=NULL;

	if(!base || !ent) return 0;		// No good
	base->entries++;				// Next entry please
	if(!base->last) 
		{
		base->first=ent;			// No entries so set fisrt
		base->last=ent;				// set last
		ent->next=NULL;				// make sure last is null
		}
	else
		{
		base->last->next=ent;		// set next entry in 
		ent->next=NULL;				// Make null
		base->last=ent;				// Set new last
		}
return 1;
}
//
// Allocate one entry and clear it out
//
PSLST AllocEnt(void)					// Get an entry	
{
PSLST hold;

	hold=malloc(sizeof(struct Slst));	// Allocate it
	if(!hold) return NULL;				// NG return 
	memset(hold,0,sizeof(struct Slst));	// Clear it out
return hold;							// Send it back
}
//
// Free all the entires in a base
//
int FreeSlst(PSLSTBASE base)
{
PSLST hold;
PSLST scan;

	if(!base) return 0;						// No base
	scan=base->first;						// Set first entry
	if(!scan) return 1;
	while(scan->next)						// while stuff to do
		{
		hold=scan->next;					// Save next
		if(scan->entry) free(scan->entry);	// If data free it
		if(scan->entry2) free(scan->entry2);// If data free it
		if(scan->entry3) free(scan->entry3);// If data free it
		free(scan);							// Free this
		scan=hold;							// Set new one
		}
return 1;
}
