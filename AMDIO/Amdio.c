/******************************************************************************
AMD Variable Record Manager Routines to maintain CSV files and records. The "KEYs"
for a AMD file is its "Field Name"/number. These can be used to read in a random
order.
******************************************************************************/
#include "windows.h"
#include "search.h"
#include "stdlib.h"
#include "stdio.h"
#include "schglobal.h"
#include "AMDIO.H"
char *FindSep(char *c,int quote);
int trim(char *data);


#pragma data_seg("Shared")
HANDLE IOMUX=NULL;			// Main IO mutex
#pragma data_seg()

PAMDB AMDRB=NULL;			// Master Block
int	  Sindex;				// Sort Index
char  DUMMY[]="";			// Dummy pointer
#pragma comment(linker,"/section:Shared,rws")

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
    return 1;
        UNREFERENCED_PARAMETER(hInst);
        UNREFERENCED_PARAMETER(ul_reason_being_called);
        UNREFERENCED_PARAMETER(lpReserved);
}
/**************************************************************************
Initalize the VRM File sctructure and Base Components
**************************************************************************/
int InitAMD()
{
DWORD dwt=0,dwl=MAX_PATH;
HKEY hkey;
long rc=0;

	if(AMDRB) return TRUE;					// Init Already  Called
	//if(!IOMUX) IOMUX=OpenMutex(MUTEX_ALL_ACCESS,TRUE,"AMDIOMUX");
	//if(!IOMUX) {
    //	IOMUX=CreateMutex(0,FALSE,"AMDIOMUX");
	//	}
	AMDRB=(PAMDB) GlobalAlloc(GPTR,sizeof(AMDB));
	if(!AMDRB) { MsgtoScreen("Unable to Build Base CB"); AMDExit(); return FALSE;}
	if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_KEY,0,KEY_READ,&hkey))!=ERROR_SUCCESS) strcpy(AMDRB->BaseDir,"C:\\AMD\\");
	else {
		RegQueryValueEx(hkey,REG_AMDLIB,NULL,&dwt,AMDRB->BaseDir,&dwl);
		RegCloseKey(hkey);
		}

return TRUE;
}
/**************************************************************************
Add a new AMDFILE to the list
**************************************************************************/
PAMDFILE AddAMD(char *fn)
{
PAMDFILE pv=NULL;
PAMDFILE pv2=NULL;

	if(!AMDRB) return NULL;					// Must Call init First
	RemoveBlanks(fn);						// Normalize the data
	WaitForSingleObject(IOMUX,INFINITE);	// Wait for other to finish
	if( (pv=isAMD(fn)) )					// is there a duplicate
		{
		pv->usecnt++;						// add 1 to use count
		ReleaseMutex(IOMUX);				// Free the hold
		return pv;							// return the found one
		}									// already there
	if(!AMDRB->BaseAMD)						// First AMD added
		{
		AMDRB->BaseAMD=AllocAMD();			// Allocate this one please
		if(AMDRB->BaseAMD) strcpy(AMDRB->BaseAMD->fname,fn);	// save file name
		ReleaseMutex(IOMUX);						// Free the hold
		return AMDRB->BaseAMD;				// return this value
		}
	pv=AMDRB->BaseAMD;						// Start of loop
	while(pv) {pv2=pv; pv=pv->Next;}		// Find last one
	if(pv2) pv=AllocAMD();					// if all Ok allocate a new one
	pv2->Next=pv;
	strcpy(pv->fname,fn);					// save file name
	ReleaseMutex(IOMUX);					// Free the hold
	return pv;								// Return NULL or adress
}
/**************************************************************************
Remove an old AMDFILE to the list
**************************************************************************/
int RemoveAMD(PAMDFILE pr)
{
PAMDFILE pv=NULL;
PAMDFILE pv2=NULL;
int found=0;

	if(!AMDRB) return 0;						// Must Call init First
	WaitForSingleObject(IOMUX,INFINITE);	// Wait for other to finish
	pv=AMDRB->BaseAMD;							// Load up first VRMFILE
	while(pv)	
		{
		if(pv==pr) {found=1; break;}			// match- send back this entry
		pv2=pv;									// save this one
		pv=pv->Next;							// next guy
		}
	if(!found) {ReleaseMutex(IOMUX);return 0;}	// Hey its not here!
	if(pv->usecnt>1) {ReleaseMutex(IOMUX);pv->usecnt--; return 1;}
	if(pv2) {pv2->Next=pv->Next;}				// Rechain this please
	else {AMDRB->BaseAMD=pv->Next;}
	ReleaseMutex(IOMUX);						// Free the hold
	CloseAMD(pv);								// close the file 
	CleanupAMDFile(pv);							// Clenup the little bits
	free(pv);									// free the main one
return 1;
}
/**************************************************************************
Create AMDFILE from scratch
**************************************************************************/
int CreateAMD(PAMDFILE pv)
{
	WaitForSingleObject(IOMUX,INFINITE);		// Wait for other to finish
	pv->fptr=fopen(pv->fname,"w+");				// Wipe out and rebuild
	ReleaseMutex(IOMUX);
	if(pv->fptr==NULL) return FALSE;			// could not open
	return 1;
}
/**************************************************************************
Create AMDFILE from scratch
**************************************************************************/
int OpenAMD(PAMDFILE pv)
{
	WaitForSingleObject(IOMUX,INFINITE);		// Wait for other to finish
	pv->fptr=fopen(pv->fname,"r+");				// Open if already there
	ReleaseMutex(IOMUX);						// Free the hold
	if(pv->fptr==NULL) return FALSE;			// could not open
	return 1;
}
/**************************************************************************
Sort An AMDFile
**************************************************************************/
int SortAMD(PAMDFILE pv,char *item,struct LineEntry *sort[])
{
struct LineEntry *li;
int i=0;													// sort index
	li=pv->Fbase;											// First Line Item
	while(li)
		{
		if(li->status) {li=li->next; continue;}				// Check status
		sort[i++]=li;										// Load up array
		li=li->next;										// Next entry Please
		}
	if( (Sindex=GetIndexOfName(pv,item))<0) return 0;		// Not found
	qsort((void *)sort,(size_t)i,sizeof(char *),(compfn)scomp);	
	return 1;
}
int __cdecl scomp(struct LineEntry *s1,struct LineEntry *s2)
{
struct LineEntry *l1;
struct LineEntry *l2;
char *c1=NULL,*c2=NULL;
	memcpy(&l1,s1,sizeof(char *));
	memcpy(&l2,s2,sizeof(char *));
	c1=FindItem(l1,Sindex);
	c2=FindItem(l2,Sindex);
	return(strcmp(c1,c2));
}
/**************************************************************************
Sort An AMDFile Assending
**************************************************************************/
int SortAMDA(PAMDFILE pv,char *item,struct LineEntry *sort[])
{
struct LineEntry *li;
int i=0;													// sort index
	li=pv->Fbase;											// First Line Item
	while(li)
		{
		if(li->status) {li=li->next; continue;}				// Check status
		sort[i++]=li;										// Load up array
		li=li->next;										// Next entry Please
		}
	if( (Sindex=GetIndexOfName(pv,item))<0) return 0;		// Not found
	qsort((void *)sort,(size_t)i,sizeof(char *),(compfn)scomp);	
	return 1;
}
int __cdecl scompa(struct LineEntry *s1,struct LineEntry *s2)
{
struct LineEntry *l1;
struct LineEntry *l2;
char *c1=NULL,*c2=NULL;
	memcpy(&l1,s1,sizeof(char *));
	memcpy(&l2,s2,sizeof(char *));
	c1=FindItem(l1,Sindex);
	c2=FindItem(l2,Sindex);
	return(strcmp(c2,c1));
}

//
// Return the data pointer for entry index
//
char *FindItem(struct LineEntry *le,int index)
{
char *c;
int x=0;
	c=le->data;										// set up the pointer
	while(x<index)
		{
		c=c+(strlen(c)+1);							// Move the pointer up
		x=x+1;										// next guy please
		}
if(c) return c;
return NULL;
}
//
// Retrieve an entry in a sorted list
//
int GetSortedItemAMD(PAMDFILE pv,struct LineEntry *from[],int num)
{
struct LineEntry *li;
int rc=1;
	li=from[num];
	if(li->status) rc=3;							// Tell caller this is deleted
	BuildItemList(pv,&pv->Curr,li);
return rc;
}
//
// Retrieve an entry in a sorted list
//
struct LineEntry *GetSortedLiAMD(PAMDFILE pv,struct LineEntry *from[],int num)
{
struct LineEntry *li;
	li=from[num];
return li;
}

/**************************************************************************
Read AMDFILE from scratch
**************************************************************************/
int UnloadAMD(PAMDFILE pv)
{
struct LineEntry *li;
char *buffer=NULL;
int x=0;

	if(!pv->fptr) return 0;									// Must Call Open First
	buffer=malloc(MAXBUF);									// Get a buffer
	if(!buffer) return 0;									// NG Get Out
	WaitForSingleObject(IOMUX,INFINITE);					// Wait for other to finish
	if(pv->records) CleanupAMDFile(pv);						// Was Unloaded Before
	fseek(pv->fptr,0l,SEEK_SET);							// Set to start of file
	fgets(buffer,MAXBUF,pv->fptr);							// Read first Line
	pv->inums=BuildItemNames(buffer,pv);					// Store Item names in CB
	pv->records=0;											// Reset Record Count
	while(fgets(buffer,MAXBUF,pv->fptr))
		{
		x=strlen(buffer); buffer[--x]='\0';					// Set up the Size
		li=malloc(sizeof(struct LineEntry));				// Get the Buffer
		if(!li) {ReleaseMutex(IOMUX); free(buffer); return FALSE;}		// OOps
		memset(li,0,sizeof(struct LineEntry));				// Clean it out
		li->data=malloc(++x);								// Get the buffer
		if(!li->data){ReleaseMutex(IOMUX); free(buffer); return FALSE;}	// No Good get out
		li->length=x;										// Save away buffer length
		strcpy(li->data,buffer);							// Copy in the Dat
		BuildLineEntry(li);									// Set up Nulls
		if(!pv->Fbase) {pv->Fbase=li; pv->Lbase=li;}		// First one
		else {pv->Lbase->next=li; pv->Lbase=li;}			// Else make it the last
		pv->records++;										// One more record
		memset(buffer,0,sizeof(buffer));					// Clear it out
		}
	if(buffer) free(buffer);								// free the buffer please
	ReleaseMutex(IOMUX);									// Free the hold
return 1;
}
/**************************************************************************
Check and see if AMD file already loaded into system
**************************************************************************/
PAMDFILE isAMD(char *fn)
{
PAMDFILE PV;

	PV=AMDRB->BaseAMD;							// Load up first VRMFILE
	while(PV)	{
		if(!stricmp(fn,PV->fname)) return PV;	// match- send back this entry
		PV=PV->Next;							// load up the next guy
		}

return NULL;
}
/**************************************************************************
Close VFMFILE from scratch
**************************************************************************/
int CloseAMD(PAMDFILE pv)
{
	if(!pv->fptr) return 1;								// Just let it go
	WaitForSingleObject(IOMUX,INFINITE);				// Wait for other to finish
	fclose(pv->fptr); pv->fptr=NULL;					// Close and set not active
	ReleaseMutex(IOMUX);
	return 1;
}
/**************************************************************************
Flush the LineEntry Data to the file
**************************************************************************/
int FlushAMD(PAMDFILE pv)
{
struct LineEntry *li;
int x;
char buffer[MAXBUF];
char workbuf[2048];											// Temp hold for names
char *c;

	if(pv->fptr) fclose(pv->fptr);							// Close the Old File
	pv->fptr=NULL;											// Reset the pointer
	WaitForSingleObject(IOMUX,INFINITE);					// Wait for other to finish
	pv->fptr=fopen(pv->fname,"w+");							// Open and destroy old
	if(!pv->fptr){ReleaseMutex(IOMUX); return 0;}			// could not open
	FlushItemNames(pv,buffer);								// Write Out First Record
	fwrite(buffer,strlen(buffer),1,pv->fptr);				// Write out Item names
	li=pv->Fbase;											// Get first entry
	while(li)
		{
		if(li->status) {li=li->next; continue;}				// Check status
		x=0;												// Resset index
		memset(buffer,0,MAXBUF);							// clear it out
		c=li->data;											// point to data
		while(pv->names[x])									// for all itemnames
			{
			if(pv->namesflg[x]) {x++; continue;}			// Item was deleted
			sprintf(workbuf,"\"%s\";",c);						// set it up
			strcat(buffer,workbuf);							// Copy over buffer
			c=c+(strlen(c)+1);								// Next item please
			x++;											// nexrt one please
			}
		buffer[strlen(buffer)-1]='\0';						// strip off last ";"
		strcat(buffer,"\n");								// put on cl/rf
		fwrite(buffer,strlen(buffer),1,pv->fptr);			// write it out
		li=li->next;										// Next line
		}
	if(pv->fptr) fclose(pv->fptr);							// Close the Old File
	pv->fptr=NULL;											// Reset pointer
	ReleaseMutex(IOMUX);
return 1;
}
//
// Update the file with the item names
//
int FlushItemNames(PAMDFILE pv,char *buffer)
{
int i=0;
char workbuf[1024];

	memset(buffer,0,MAXBUF);								// clear it out
	while(pv->names[i])										// For all the Names
		{
		if(pv->namesflg[i]) {i++; continue;}				// Item was deleted
		sprintf(workbuf,"%s;",pv->names[i]);				// Add in names + ";"
		strcat(buffer,workbuf);								// Concat please
		i++;												// Next Guy
		}
	buffer[strlen(buffer)-1]='\0';							// Take off last ";"
	strcat(buffer,"\n");									// Add a Cl/rf
return 1;
}
/**************************************************************************
 Utility Section of the module performs usefull but unimpressive functions
**************************************************************************/
void AMDExit(void)
{
PAMDFILE PV;
PAMDFILE PVH;
HGLOBAL hg=0;
	if(AMDRB && AMDRB->BaseAMD)				// Lets see what we have
		{
		PV=AMDRB->BaseAMD;					// Load up first VRMFILE
		while(PV) {
			if(PV->fptr) CloseAMD(PV);		// Flush and close file
			else PV->usecnt=0;				// else get rid of it
			PVH=PV->Next;					// save the next;
			if(!PV->usecnt) {				// if last call to exit
				CleanupAMDFile(PV);			// Clenup the little bits
				free(PV);					// free the main one
				}
			PV=PVH;							// reload and go
			}
		}
	GlobalFree(AMDRB);
	return;
}
// Send a message to the screen
int MsgtoScreen(char *text)
{
	MessageBox(NULL, text, NULL, MB_OK | MB_ICONHAND);
    return 1;
}
// Allocate a structure for the VRM
PAMDFILE AllocAMD()
{
PAMDFILE pv;

	if(NULL==(pv=malloc(sizeof(struct AMDFILE)))) return NULL;
	memset(pv,0,sizeof(struct AMDFILE));
	pv->Curr.buffer=malloc(MAXBUF);								// Allocate Line Item buffer
	pv->usecnt=1;												// Set use count to 1
	return pv;
}
// Cleanup the peices
void CleanupAMDFile(PAMDFILE PV)
{
struct LineEntry *li;
struct LineEntry *l2;
int x=0;

	if(PV->Curr.buffer) free(PV->Curr.buffer);
	if(PV->inames) free(PV->inames);								// Free up item names
	PV->inames=NULL;												// Reset this value
	if(!PV->Fbase) return;											// No Line entryes
	li=PV->Fbase;													// Seed the list
	while(li->next)
		{
		l2=li->next;												// Save the next pointer
		if(li->data) free(li->data);								// free the buffer
		free(li);													// free the entry
		li=l2;														// Set next to current
		}
	PV->Fbase=NULL;
	PV->Lbase=NULL;
return;
}
// From a char field, Take out leading and trailing blanks
int RemoveBlanks(char *str)	
{
int x=0;

	x=strlen(str); if(x==0) return x;					// Nothing to do	
	x--;												// leave off one
	while(str[0]==' ') {x--; strcpy(&str[0],&str[1]);}	// if not a char field we will allow <' ' chars
	while(str[x]==' ') x--;								// update with a null;
	str[++x]='\0';										// put in the null
	return (x);
}
//
// Setup LineEntry On Unload (from raw data file)
//
int BuildLineEntry(struct LineEntry *li)
{
int i,quote=0;
char *c,*p;

	c=li->data;												// Start searching Here
	for( i = 0; i <= MAXITEMS; i++)							// Go for MAX ITEMS
		{
		if(*(c)=='\"') {strcpy(c,c+1); quote=1;}			// this is quoted
		else quote=0;										// not quoted
		p=FindSep(c,quote);									// Find a seperator
		if(!p) return i;									// All done thanks
		if(*(p) == '\"') {									// quoted string 
			strcpy(p,p+1);									// remove the quote
			*(p)='\0';										// chop it off
			c=p+1;											// go past it please
			continue;
			}
		if(*(p)==';') {
			*(p)='\0';										// Just end it please
			c=p+1;											// go past it
			continue;										// next field
			}
		break;												// if we get here I give up
		}
return i;													// Number Found
}
//
// Find a seperator even if it is quoted
//
char *FindSep(char *c,int quote)
{
	while(*(c)) {
		if(*(c)=='\"' && *(c+1)<' ') {*(c)='\0'; return NULL;}
		if(*(c)=='\"' && *(c+1)==';' && quote) return c;				// check both Quote and ;
		if(*(c)==';' && !quote) return c;					// now just check ;
		c++;
		}
return NULL;
}
//
// Copy a LineEntry to a Line Items
//
int BuildItemList(PAMDFILE pv,struct LineItems *li,struct LineEntry *le)
{
int i;
char *c;

	memcpy(li->buffer,le->data,le->length);					// copy over the data
	c=li->buffer;												// Start searching Here
	for( i = 0; i <= (int)pv->inums; i++)					// Go for MAX ITEMS
		{
		li->items[i]=c;										// Set into this Item	
		c=c+strlen(c)+1;									// Move to Next one
		}
return i;													// Number Found
}
int BuildItemNames(char *buffer,PAMDFILE pv)
{
int i;
char *c,*p;

	i=strlen(buffer);										// Get the length
	pv->inames=malloc(i);									// Get the storage
	if(!pv->inames) return 0;								// NG Get out
	buffer[--i]='\0';										// strip off CR/LF
	strcpy(pv->inames,buffer);								// Save into CB
	c=pv->inames;											// Start searching Here
	for( i = 0; i <= MAXITEMS; i++)							// Go for MAX ITEMS
		{
		p=strstr(c,DEFAULTSEP);								// Find a seperator
		if(p) *(p)='\0';									// Put in a NULL
		pv->names[i]=c;										// Set into this Item	
		if(!p) break;										// Last One Thanks
		c=p+1;												// Move to Next one
		}
return i;
}
//
// Retuens the Index of an item name
//
int GetIndexOfName(PAMDFILE pv,char *item)
{
int i=0;
	while(pv->names[i])										// For all the Names
		{
		if(strcmp(pv->names[i],item)==0) return i;			// Found it 
		i++;												// Next Guy
		}
return -1;													// Not Found
}
//
// Make PV.Current record the recnum
//
int Select(PAMDFILE pv,int recnum)
{
struct LineEntry *li;
int i=0;					
	li=pv->Fbase;											// Start of the list
	while(li)
		{
		if(i==recnum) break;								// Found it
		i++; li=li->next;									// Next one
		}
	if(li) BuildItemList(pv,&pv->Curr,li);						// Make it current
return 0;												// Not Found
}
int Find(PAMDFILE pv,struct LineEntry *li,char *field,char *data,int len)
{
struct LineEntry *ls;
int fldidx=0;

	if( (fldidx=GetIndexOfName(pv,field))<0) return 0;		// Field not found
	if(li) {ls=li; ls=ls->next;}							// Find Next ???
	else ls=pv->Fbase;										// Start at the top
	if(!len) len=strlen(data);								// If no Length given
	while(ls)
		{
		BuildItemList(pv,&pv->Curr,ls);							// Load it up
		if(strnicmp(pv->Curr.items[fldidx],data,len)==0) return 1; // Found it
		ls=ls->next;										// Next entry Please
		}
return 0;
}
//
// Find a data match in a sorted list
//
int FindSorted(PAMDFILE pv,struct LineEntry *list[],int index,char *field,char *data,int len)
{
struct LineEntry *ls;
int fldidx=0;
char buffer[512];

	if( (fldidx=GetIndexOfName(pv,field))<0) return -1;				// Field not found
	if(!len) len=strlen(data);										// If no Length given
	if((unsigned int)index > pv->records) return -1;				// No good
	if(index<0) index=0;											// Just in case
	ls=list[index];
	while((unsigned long)index < pv->records)
		{
		BuildItemList(pv,&pv->Curr,ls);									// Load it up
		strncpy(buffer,pv->Curr.items[fldidx],sizeof(buffer)-1);
		trim(buffer);
		if(strnicmp(buffer,data,len)==0) return index;				// Found it
		if(strnicmp(buffer,data,len)>0) return -1;					// Sorted list and we are beyond what we are looking for
		index++;													// Next One Please	
		if((unsigned int)index > pv->records) break;				// No more records	
		ls=list[index];												// Next entry Please
		}
return -1;
}
//
// Add in One item to this File
//
int CopyItemsAMD(PAMDFILE pv1,PAMDFILE pv2)
{
int i;
char buffer[MAXBUF];
char workbuf[4096];

	memset(buffer,0,MAXBUF);									// clear it out
	i=0;
	WaitForSingleObject(IOMUX,INFINITE);					// Wait for other to finish
	while(pv1->names[i])										// For all the Names
		{
		sprintf(workbuf,"%s;",pv1->names[i]);					// Add in names + ";"
		strcat(buffer,workbuf);									// Concat please
		i++;													// Next Guy
		}
	if(pv2->inames) { free(pv2->inames); pv2->inames=NULL; }	// free old buffer
	pv2->inums=BuildItemNames(buffer,pv2);						// Store Item names in CB
	ReleaseMutex(IOMUX);
return 1;
}
//
// Add in One item to this File
//
int AddItemAMD(PAMDFILE pv,char *field)
{
int i;
struct LineEntry *li;
char *buffer;
char workbuf[1024];

	if( (i=GetIndexOfName(pv,field))>=0) return 1;				// Field already there
    buffer=malloc(MAXBUF);										// Get a buffer
	if(!buffer) return 0;										// opps
	i=pv->inums; i++;											// Set index for next one
	li=pv->Fbase;												// Start at the top
	memset(buffer,0,MAXBUF);									// clear it out
	i=0;
	while(pv->names[i])											// For all the Names
		{
		sprintf(workbuf,"%s;",pv->names[i]);					// Add in names + ";"
		strcat(buffer,workbuf);									// Concat please
		i++;													// Next Guy
		}
	strcat(buffer,field); strcat(buffer,";");					// Add in the new field
	free(pv->inames); pv->inames=NULL;							// free old buffer
	pv->inums=BuildItemNames(buffer,pv);						// Store Item names in CB
	free(buffer);												// get rid of it
return 1;
}

int DeleteItemAMD(PAMDFILE pv,char *field)
{
int i;

	if( (i=GetIndexOfName(pv,field))<0) return 0;				// Field not found
	pv->namesflg[i]=1;											// Mark deleted
return 1;
}
int UpdateAMD(PAMDFILE pv,struct LineEntry *li,char *field,char *data)
{
int length,i,x;
char *newbuf,*c,*p;
char buf[256];

	memset(buf,0,sizeof(buf));	
	if( (i=GetValueAMD(pv,li,field,buf,256))<0) return 0;		// Field not found
	if( (strlen(data)) > (strlen(buf)) )
		length=(li->length+strlen(data)+1);						// Get length of new buiffer
	else 
		length=li->length;										// just use the old one
	if(!(newbuf=malloc(length))) return 0;						// Get a new buffer
	memset(newbuf,0,length);									// Clear it out
	c=newbuf;													// Set up our pointer
	p=li->data;	
	WaitForSingleObject(IOMUX,INFINITE);					// Wait for other to finish
	for(x=0;x<(int)pv->inums;x++)								// Copy in all of the Data
		{	
		if(x==i) strcpy(c,data);								// New Fiels Update
		else strcpy(c,p);										// Copy in the item
		c=c+strlen(c)+1;										// Move to next position
		p=p+strlen(p)+1;										// Next old data position
		}
	if(x==i) strcpy(c,data);									// Get the last field
	else strcpy(c,p);											// Get the last field 
	if(length==li->length){
		memcpy(li->data,newbuf,length);							// Copy it back Please
		free(newbuf);
		}	
	else {
		free(li->data);											// Old Buffer
		li->data=newbuf;										// Set new buffer
		li->length=length;										// Update New Length
		}
	ReleaseMutex(IOMUX);
return 1;
}
struct LineEntry * AllocLiAMD(PAMDFILE pv,int length)
{
struct LineEntry *li;

		li=malloc(sizeof(struct LineEntry));				// Get the Buffer
		if(!li) return NULL;								// OOps
		memset(li,0,sizeof(struct LineEntry));				// Clean it out
		li->data=malloc(length);							// Get the buffer
		if(!li->data) return NULL;							// No Good get out
		li->length=length;									// Save away buffer length
return li;
}

int AddLiADM(PAMDFILE pv)
{
struct LineEntry *li;
char workbuf[1024];

		memset(workbuf,0,1024);								// clear it out
		memset(workbuf,';',pv->inums);						// Set up dummy buffer
		li=malloc(sizeof(struct LineEntry));				// Get the Buffer
		if(!li) return FALSE;								// OOps
		memset(li,0,sizeof(struct LineEntry));				// Clean it out
		li->data=malloc(strlen(workbuf)+1);					// Get the buffer
		if(!li->data) return FALSE;							// No Good get out
		li->length=strlen(workbuf);							// Save away buffer length
		strcpy(li->data,workbuf);							// Copy in the Data
		BuildLineEntry(li);									// Set Up the Pointers
		if(!pv->Fbase) {pv->Fbase=li; pv->Lbase=li;}		// First one
		else {pv->Lbase->next=li; pv->Lbase=li;}			// Else make it the last
		pv->records++;										// increment number of records
return 1;
}
int DeleteLiADM(PAMDFILE pv,struct LineEntry *li)
{
		li->status=1;										// Delete this entry
		pv->records--;										// increment number of records
return 1;
}
int GetValueAMD(PAMDFILE pv,struct LineEntry *li,char *field,char *data,int length)
{
int i,x;
char *c;

	if( (i=GetIndexOfName(pv,field))<0) return -1;			// Field not found
	c=FindItem(li,i);										// point to data
	x=strlen(c);											// set copy length
	if (length < x) x=(length-1);							// Only copy what you can
	strncpy(data,c,x);										// Copy over th data

return i;
}
struct LineEntry *CopyAMD(PAMDFILE pv,struct LineEntry *from)
{
struct LineEntry *l2;
int scan=0,x=0;

	WaitForSingleObject(IOMUX,INFINITE);				// Wait for other to finish
	l2=malloc(sizeof(struct LineEntry));				// Allocate it please
	if(!l2) {ReleaseMutex(IOMUX);return NULL;}			// no memory NG
	memset(l2,0,sizeof(struct LineEntry));				// clear it out
	l2->data=malloc(from->length);						// Get storage for it
	if(!l2->data) {free(l2); ReleaseMutex(IOMUX);return NULL;}	// Ooops
	memcpy(l2->data,from->data,from->length);			// Copy Old data
	l2->length=from->length;							// make the length the same
	ReleaseMutex(IOMUX);
return l2;
}
int trim(char *data)
{
int x=0;

	x=strlen(data);
	if(x<1) return 0;
	x--;
	while((data[x]<=' ') && x) {data[x]=0; x--;}
	while((data[0])== ' ') strcpy(&data[0],&data[1]);
return x;
}
