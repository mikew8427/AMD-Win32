#include "windows.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "winsetup.h"
#include "W32TRACE.h"
#include "admtrie.h"
#include "crc.h"
#include "slst.h"
#include "parmblk.h"
#include "routine.h"
#include "winutil.h"
/************************************************/
int WINAPI WinMain(	HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdLine,int nCmdShow)
{
char rout[]="main";
int x=0,rc=0;

   WinSetup(hInstance,lpszCmdLine,"W32UTIL Script",SW_HIDE);

   if(!argc[1]) {printf("No Script name provided\n"); exit (8);}
   SetupWorkDir();														// Set up our current working dir
   if(!Setup(argc[1])) {printf("Unable to Start Script\n"); exit(8);}	// Setup Failed
   if(argc[2] && strnicmp(argc[2],"/D",2)==0) 
		{
	    SetLogType(MSG_DEBUG);	// Reset Log type 
		DumpScript(CURRENT);	// Dump out lines	    	
		}
   SetArgs();
   InitLstBase(&Vbase[VbaseNext++],CURRENT->sbase);						// Init the Var Base entry
   LoadVbase(CURRENT,"BASE");											// Base Variables
   LoadVbase(CURRENT,"LOCAL");											// Local Variables
   lmsg(MSG_INFO,"Start of Execution with module name [%s]",argc[1]);	// Send out message
   rc=ExecAtLevel(CURRENT,0,1,CURRENT->lfirst);							// Start execution script
   lmsg(MSG_INFO,"End   of Execution with module name [%s]",argc[1]);	// Send out message
   FreeScript(CURRENT);													// Free the storage
   for(x=0;x<VbaseNext;x++) FreeSlst(&Vbase[x]);						// Free Variable Pools 
   free(CURRENT);														// Free up the ps
   free(pb);															// Free the parmblk
return rc;
}  /* End of WINUIL */
//
// Process the input args and make variables from them
//
//
void SetArgs(void)
{
int x=1;
char vname[8];
    LoadVbase(CURRENT,"UTIL");											// Base Variables
	while(argc[x] ) {
		sprintf(vname,"IPARM%d",x);
		setvalue("UTIL",vname,argc[x]);
		if(stricmp(argc[x],"/D")==0) {
			SetLogType(MSG_DEBUG);	// Reset Log type 
			DumpScript(CURRENT);	// Dump out lines	    	
			}
		x++;
		}
}
// Set our wroking dir to our EXE Path
//
void SetupWorkDir(void)
{
char buffer[_MAX_PATH+1];

	GetModuleFileName((HMODULE)NULL,buffer,_MAX_PATH);
   _splitpath(buffer,fdrive,fdir,fname,fext);								// Use Global vars for split
   strcpy(buffer,fdrive);strcat(buffer,fdir);
   if(buffer[strlen(buffer)-1]=='\\') buffer[strlen(buffer)-1]='\0';
   SetCurrentDirectory(buffer); 
}
int Setup(char *name)
{
int x=0;

   _splitpath(name,fdrive,fdir,fname,fext);								// Use Global vars for split	
   OpenLog(fname);														// Open Default log
   InitLstBase(&Pbase,"Procs");											// init proc base
   InitLstBase(&Lbase,"Labels");										// Init list base
   ProcNumber=1;														// Set base proc number
   pb=malloc(sizeof(struct parmblk));									// Allocate the parm block
   memset(pb,0,sizeof(struct parmblk));									// Clear it out
   for(x=0; x<MAXLVBASE; x++) memset(&LVBase[x],0,sizeof(struct VentBase)); 
   CurrentLVBase=0;														// Current position in Base
   if(!(CURRENT=BuildScript(name))) {lmsg(MSG_ERR,"Unable to build structure for Script"); return 0;}
   if(!LoadScript(CURRENT)) {lmsg(MSG_ERR,"Unable to unload Script [%s]",name); return 0;}
   pb->curr_ps=CURRENT;
   strcpy(CURRENT->sbase,fname);										// Default variable pool

return 1;
}
//
// Allocate and build the next Script Block
//
PSCR BuildScript(char *name)
{
PSCR NEW=NULL;															// New Pointer
PSCR scan=NULL;															// Scan Pointer

	NEW=malloc(sizeof(struct Script));									// Allocate it please
	if(!NEW) return NULL;												// No Good
	memset(NEW,0,sizeof(struct Script));								// Clear it out
	ScriptLevel++;														// Increment Level
	NEW->level=ScriptLevel;												// This Level
	strcpy(NEW->sname,name);											// The File Name
	if(!BASE) {BASE=NEW; return NEW;}									// Set up BASE
	scan=BASE;															// Set value
	while(1)
		{
		if(!scan->next) {scan->next=NEW; NEW->prev=scan; break;}		// chain em up
		scan=scan->next;									
		}
	return NEW;
}
//
// Release all storage for Script
//
int FreeScript(PSCR ps)
{
PLENT last=NULL;															// last entry
PLENT next=NULL;															// Save entry	
PSCR  p1=NULL;

	last=ps->lfirst;														// Set up to free lines
	while(last)
		{next=last->next; 
		free(last->line); 
		free(last); 
		last=next;}	// Free all lines
	if(ps->next)															// Free from middle
		{
		p1=ps->prev;														// Set previous one
		p1->next=ps->next;													// Set new next
		p1=ps->next;														// Load up next 
		p1->prev=ps->prev;													// Make this ones prev
		}
return 1;
}
//
// Given a Script Block read in the file and build a link list
//
int LoadScript(PSCR ps)
{
char *buffer;															// Buffer for read
PLENT work=NULL;														// Work entry
PLENT last=NULL;														// last entry	
int x=0,linenum=0,level=0;
int currentp;
PSLST lent;																// List entry

	currentp=1;															// Set the proc number
	if(ps->sname[0]=='\0') return 0;
	ps->sfile=fopen(ps->sname,"r");										// Open it up
	buffer=malloc(DATABUFSIZE);											// Get file buffer
	if(!ps->sfile) return 0;											// NG Get out
	while(GetNextLine(buffer,DATABUFSIZE,&linenum,ps))					// Start to read it in
		{
		if(!setline(buffer)) continue;									// Set up the line
		work=malloc(sizeof(struct Lineent));							// get a new line
		if(!work) {lmsg(MSG_ERR,"Unable to build Instorage Script"); return 0;}
		memset(work,0,sizeof(struct Lineent));							// Clear it out
		work->num=linenum;												// Put into entry
		work->line=malloc(strlen(buffer)+1);							// get storage for line
		if(work->line) strcpy(work->line,buffer);						// save it away
		if(last) {work->prev=last; last->next=work; last=work;}			// add to the end
		else {last=work;ps->lfirst=work;}								// First guy
		if(strnicmp(work->line,"IF",2)==0) level ++;
		if(strnicmp(work->line,"Else",4)==0 && level==0)
			{
			SetLogType(MSG_DEBUG);											// Reset Log type 
			lmsg(MSG_ERR,"Line [%d] Else dose not match to IF",linenum);
			return 0;
			}
		if(strnicmp(work->line,"While",5)==0) level++;					// On while inc level
		if(strnicmp(work->line,"PROC:",5)==0) 
			{
			currentp=++ProcNumber;										// New proc number
			lent=AllocEnt();											// Get new entry
			if(lent) {lent->entry=(void *)work; AddSlst(&Pbase,lent);}	// Add ti list 
			}
		if(strnicmp(work->line,":",1)==0) 
			{
			lent=AllocEnt();											// Get new entry
			if(lent) {lent->entry=(void *)work; AddSlst(&Lbase,lent);}	// Add to list
			}
		work->level=level;												// Set into level
		work->proclevel=currentp;										// Set proc level
		if(strnicmp(work->line,"ENDP",4)==0) {currentp=1;}				// Reset Proc Level
		if(stricmp(work->line,"END")==0) level--;						// On End STM dec level
		if(stricmp(work->line,"END;")==0) level--;						// On End STM dec level
		memset(buffer,0,DATABUFSIZE);
		}
	if(level != 0) 
		{
		SetLogType(MSG_DEBUG);											// Reset Log type 
		lmsg(MSG_ERR,"Invalid IF/END Clause Match");					// Send out message
		DumpScript(ps);													// Dump the script
		FreeScript(ps);													// Free the storage
		linenum=0;														// No lines
		}
	if(ps->sfile) {fclose(ps->sfile); ps->sfile=NULL;}					// Close the file
return linenum;															// return number of lines
}
int GetNextLine(char *buffer,int size,int *linenum,PSCR ps)							// Build a complete Line
{
char hold[DATABUFSIZE];
int x;

	memset(hold,0,DATABUFSIZE);											// Clear it out
	memset(buffer,0,size);
	if(fgets(hold,DATABUFSIZE,ps->sfile) == NULL) return 0;				// Get a line
	if ( (x=FindLastString(hold,"//"))>0) hold[x]='\0';					// Is there a comment
	trim(hold);
	*linenum=*linenum+1;
	while(hold[strlen(hold)-1]==',')									// While line ends in a ,
		{
		strcat(buffer,hold);											// Concat the line
		fgets(hold,DATABUFSIZE,ps->sfile);								// Read in the file
		if ( (x=FindLastString(hold,"//"))>0) hold[x]='\0';				// Is there a comment
		trim(hold);														// strip off blanks
		*linenum=*linenum+1;											// increment linenumber
		}
	strcat(buffer,hold);												// Put last piece at end
return 1;
}
int FindLastString(char *data,char *what)
{
int x;
	x=strlen(data);
	while(x) { if(strnicmp(&data[x],what,strlen(what))==0) return x; x--;}
return 0;
}
//
// Given a Script Block Dump all the lines to log
//
int DumpScript(PSCR ps)
{
PLENT work;

	work=ps->lfirst;													// Load up first line
	while(work)
		{
		lmsg(MSG_DEBUG,"Line[%.4d] Level[%.3d] Proc[%.2d] STM[%s]",work->num,work->level,work->proclevel,work->line);
		work=work->next;
		}
return 1;
}
//
// Execute each line you have to
//
int	ExecAtLevel(PSCR ps,int level,int plevel,PLENT l)
{
PLENT nl;
char line[MAXLINE];
int routnum=0;
int i;

	ps->lnext=l;																// Start here please
	ps->level=level;															// Current level
	ps->plevel=plevel;															// Current Proc level
	while(1)
		{
		DispMsg();
		if(ps->fatal) return ps->fatal;											// Get out of town 
		nl=ps->lnext;															// Execute This line
		if(!nl) return 1;														// Al done with this
		if(nl->level>level){ExecAtLevel(ps,nl->level,plevel,nl); nl=ps->lnext;} // If new Line Level
		if(!nl) return 1;														// end of script
		if(nl->level<level) return 1;											// Bounce up
		if(nl->proclevel!=plevel) {UpdateLocalVars(ps);ps->lnext=ps->preturn; return 1;}// Outside of our proc
		memset(line,0,MAXLINE);													// Clear out buffer
		strcpy(line,nl->line);													// Copy it over
		if(stricmp(line,"ENDP")==0){UpdateLocalVars(ps);ps->lnext=ps->preturn; return 1;}// Outside of our proc
		ps->lnext=nl->next;														// Load up next line
		if(line[0]=='*'|| line[0]==':'|| (strncmp(line,"//",2)==0)) continue;	// Comment line or label
		for(i=0;i<MAXPARMS;i++){pb->parms[i]=parmv[i];}							// Make sure we start here
		memset(&parmv,0,(ParmvSize*MAXPARMS));									// Clear it out
		ParseRoutine(line,pb);													// Parse the line
		if(stricmp(pb->parms[0],"ExitProcess")==0) return (atoi(pb->parms[1]));	// Return with RC
		if( (routnum=isroutine(pb->parms[0],pb))<0) 
			{
			lmsg(MSG_ERR,"Routine not Supported [%s]",pb->parms[0]);			// Tell them about it
			continue;															// Just bypass for now
			}
		}
	free(pb);																	// Free Parm Block
return 0;
}
//
// Update Local vars from Current to Current -1
//
int	UpdateLocalVars(PSCR ps)										
{
PVENT ent1;
PVENT ent2;
int saveold=CurrentLVBase;
	
	ent1=LVBase[CurrentLVBase].first;											// Load first entry
	CurrentLVBase--;															// Back up one
	while(ent1)
		{
		ent2=ent1->next;														// Load up next
		if(!ent1->ref) {VentFree(ent1); ent1=ent2; continue;}					// Nope skip it
		VentSet(ent1->name,ent1->len,ent1->data);								// Update var
		VentFree(ent1);															// Release storage
		ent1=ent2;																// load up
		}
	memset(&LVBase[saveold],0,sizeof(struct VentBase));							// clear it out
return 1;
}
int VentFree(PVENT ent)
{
	if(ent->data) free(ent->data);
	if(ent) free(ent);
return 1;
}
//
// Loads up a VDF File into a link list
//
int LoadVbase(PSCR ps, char *name)
{
char *buffer;
char fullname[_MAX_PATH+1];
PSLST lent;															// List entry
int tb;																// pointer to This base
char *c;															// work char pointer
int base=-1;

    _splitpath(name,fdrive,fdir,fname,fext);						// Use Global vars for split
	base=FindVbase(&Vbase[0],fname);								// Dose it exits??
	if(base>=0) return 1;											// Yes do not reload
	buffer=malloc(DATABUFSIZE);										// Allocate the Read buffer
	if(!buffer) {lmsg(MSG_ERR,"Unable to allocate buffer of variable read");return 0;}
	tb=VbaseNext++;													// Save the next base entry
	InitLstBase(&Vbase[tb],fname);									// Init the Var Base entry
	if(!fext[0]) strcpy(fext,".vdf");								// Variable Def File Extention
	if(!fdrive[0])	strcpy(fullname,GetPathInfo("VARIABLES"));		// Get the path for variable files
	strcat(fullname,fname); strcat(fullname,fext);					// Build the rest
	lmsg(MSG_DEBUG,"Open for VDF full file name is [%s]",fullname); // put to log
	ps->sfile=fopen(fullname,"r");									// Open it up
	if(!ps->sfile) {lmsg(MSG_ERR,"Unable to open VDF [%.100s]",fullname); return 0;}
	while( (fgets(buffer,DATABUFSIZE,ps->sfile))!=NULL)					// Start to read it in
		{
		trim(buffer);												// Get rid of blanks
		c=strstr(buffer,"=");										// find the =
		if(!c) continue;											// invalid one please skip
		*(c)=0; c++;												// Put in a NULL and pass it up
		lmsg(MSG_DEBUG,"Var/Data is V[%s] D[%s]",buffer,c);			// Put to Log
		lent=AllocEnt();											// Get new entry
		if(!lent) {lmsg(MSG_ERR,"Unable to allocate Block for Var"); return 0;}													
		lent->entry=malloc(strlen(buffer)+1);						// Get into var name
		lent->entry2=malloc(strlen(c)+1);							// Get for data 
		strcpy(lent->entry,buffer);									// Copy in var
		strcpy(lent->entry2,c);										// Copy in data
		AddSlst(&Vbase[tb],lent);									// Add to list
		}
if(buffer) free(buffer);											// Free work area
if(ps->sfile) fclose(ps->sfile);									// Close file
return 1;	
}
//
// Set a Variable to a VDF In storage List
//
int setvalue(char *base,char *var,char *value)
{
int offset=-1;
PSLST work;																// List entry
char hold[MAXVNAME]="";													// Hold area for both names
char lbase[MAXBASE]="BASE";												// Default base

	if(base && base[0]!='\0') strcpy(lbase,base);						// Use default or one provided
	offset=FindVbase(&Vbase[0],lbase);									// Dose it exits??
	if(offset<0) return 0;												// No Must load it first
	work=Vbase[offset].first;											// Get first entry
	while(work)
		{
		if(stricmp(work->entry,var)==0)									// We found the Var 
			{
			free(work->entry2);											// Get rid of Old data
			work->entry2=malloc(strlen(value)+1);						// Alloc for data 
			strcpy(work->entry2,value);									// Copy for data
			return 1;
			}				
		work=work->next;												// Next entry
		}
	// Not found above so just add it in As we go
	work=AllocEnt();													// Get new entry
	if(!work) {lmsg(MSG_ERR,"Unable to allocate Block for Var"); return 0;}													
	work->entry=malloc(strlen(var)+1);									// Get into var name
	work->entry2=malloc(strlen(value)+1);								// Get for data 
	strcpy(work->entry,var);											// Copy in var
	strcpy(work->entry2,value);											// Copy in data
	AddSlst(&Vbase[offset],work);										// Add to list
return 1;
}
//
// Save a VDF File out to disk
//
int SaveVbase(PSCR ps, char *name)
{
char *buffer;
char fullname[_MAX_PATH+1];
PSLST work;															// List entry
int base=-1;

    _splitpath(name,fdrive,fdir,fname,fext);						// Use Global vars for split
	base=FindVbase(&Vbase[0],fname);								// Dose it exits??
	if(base<0) return 1;											// Not Loaded
	buffer=malloc(DATABUFSIZE);										// Allocate the Read buffer
	if(!buffer) {lmsg(MSG_ERR,"Unable to allocate buffer of variable read");return 0;}
	if(!fext[0]) strcpy(fext,".vdf");								// Variable Def File Extention
	if(!fdrive[0])	strcpy(fullname,GetPathInfo("VARIABLES"));		// Get the path for variable files
	strcat(fullname,fname); strcat(fullname,fext);					// Build the rest
	lmsg(MSG_DEBUG,"Open for VDF full file name is [%s]",fullname); // put to log
	ps->sfile=fopen(fullname,"w");									// Open it up
	if(!ps->sfile) {lmsg(MSG_ERR,"Unable to open VDF [%.100s]",fullname); return 0;}
	work=Vbase[base].first;											// Get first entry
	while(work)
		{
		strcpy(buffer,work->entry);									// Load in Variable name
		strcat(buffer,"=");											// Equal Sign
		strcat(buffer,work->entry2);								// Load up data
		strcat(buffer,"\n");										// Put in New Line
		fputs(buffer,ps->sfile);									// Write it back out
		work=work->next;											// Next entry please
		}
if(buffer) free(buffer);											// Free it up
if(ps->sfile) fclose(ps->sfile);									// Close file
return 1;	
}
int FindVbase(PSLSTBASE vbase,char *name)
{
int x=0;
	for(x=0; x<VbaseNext; x++)
		{
		if(stricmp(Vbase[x].name,name)==0) return x;
		}
return -1;
}
int Resolve(char *data)
{
char *c,*c2;														// Pointers for Vars
int len=0,x=0;															// Lenth of name
char bname[MAXBASE];												// Base Name
char vname[MAXVAR];													// Variable Name
char hold[MAXVNAME];												// Hold area for both names
char line[MAXLINE];													// Hold area for line

	if(data[0]=='\"' && data[strlen(data)-1]=='\"')					// Quoted string 
		{
		strcpy(line,&data[1]);										// Remove Double quotes
		line[strlen(line)-1]='\0';									// This will give us 
		strcpy(data,line);											// a Resolved Literal
		return 0;													// Back to caller
		}
	if((c=strstr(data,"<"))==NULL) return 1;						// Nothing to do
	while(c)
		{
		memset(hold,0,sizeof(hold));								// Clear it out
		memset(vname,0,sizeof(vname));								// Clear it out
		memset(bname,0,sizeof(bname));								// Clear it out
		memset(line,0,sizeof(line));								// Clear it out
		c2=strstr(data,">");										// Find the back end
		if(!c2) return 1;											// Not a var
		len=c2-(c+1);												// Get the length
		strncpy(hold,(c+1),len);									// Put name in Hold area
		splitvar(hold,bname,vname);									// Separate Var from base
		findvalue(hold,bname,vname);								// Go get value ino hold
		strncpy(line,data,(c-data));								// Copy first part
		strcat(line,hold);											// Now the value
		strcat(line,++c2);											// Copy over rest
		strcpy(data,line);											// put back into data
		c=strstr(data,"<");											// Next up please
		}
	for(x=0;x<strlen(data); x++) {
		if(data[x]=='{') data[x]='<';
		if(data[x]=='}') data[x]='>';
		}
return 0;
}
void splitvar(char *full,char *base,char *var)						
{
char *c;

	c=strstr(full,".");												// Find a Dot ??
	if(!c) {base[0]=0; strcpy(var,full); return; }					// Only a var name
	*(c++)=0;														// stomp on it
	strcpy(base,full);												// Copy in base
	strcpy(var,c);													// Variable name
return;
}
void findvalue(char *full,char *base,char *var)						
{
int basenum=-1;
PVENT ent=NULL;														// Local var entry
int copylen=MAXVNAME;												// default copy length

	memset(full,0,MAXVNAME);										// Clear it out
	if(strlen(base)>0)												// We were given a base
		{
		LoadVbase(CURRENT,base);									// Load it if needed
		basenum=FindVbase(&Vbase[0],base);							// Set the base
		if(basenum<0) return;										// Cant set it
		getvalue(&Vbase[basenum],var,full);							// Set value for this guy
		return;
		}
	for(basenum=0; basenum<VbaseNext; basenum++)
		{
		getvalue(&Vbase[basenum],var,full);							// Try this guy
		if(full[0]>=' ') return;									// found one
		}
	ent=FindVentName(var);											// Local Variable??
	if(!ent) return;												// Not found
	if(copylen>ent->len) copylen=ent->len;							// Max Copy
	memcpy(full,ent->data,copylen);									// Do the deed
return;
}
void getvalue(PSLSTBASE base,char *var,char *value)
{
PSLST work=NULL;														// Work entry
char *v,*d;

	work=base->first;													// Get first entry
	while(work)
		{
		v=work->entry; d=work->entry2;									// Set up var and data pointers
		if(stricmp(v,var)==0) {strcpy(value,d); return;}				// If varthe copy data
		work=work->next;												// Next entry
		}
return;
}
//
// Find the line that has the Label "name"
//
PLENT FindLabel(PSCR ps,char *name)
{
PLENT work;

	work=ps->lfirst;													// Load up first line
	while(work)															// while we have lines
		{
		if(stricmp(work->line,name)==0) return work;					// Found it
		work=work->next;												// Load up the next
		}
return NULL;															// Not found
}
int setline(char *buffer)
{
int x=0,y=0;
char *buff2;

	x=0;																// Set starting value
	while(buffer[x]<=' ' && buffer[x]>'\0') x++;						// Remove Blanks at the front
	if(x>0) strcpy(buffer,&buffer[x]);									// Remove Blanks at front
	x=trim(buffer);														// Find Length and remove blanks
	if(buffer[0]<' ') return 0;											// No Data on line
	buff2=malloc((size_t)x+2);											// allocate Buffer
	if(!buff2) return 0;												// Can't allocate it
	memset(buff2,0,(size_t)x+2);										// Clear it out
	x=0;y=0;															// Reset values
	while(buffer[x])													// while we have data
		{
		if(buffer[x]=='\"')												// If a double quote
			{
			buff2[y++]=buffer[x++];										// Move it over
			while(buffer[x]!='\"'&& buffer[x]>'\0') buff2[y++]=buffer[x++];// While within string
			}	
		if(buffer[x]>=' ') buff2[y++]=buffer[x++];						// Move it over
		else x++;														// Else move on
		}
	strcpy(buffer,buff2);												// copy it back over
	free(buff2);														// Free up the memory
return(strlen(buffer));													// Send it back
}
//
// Add a Var Entry
//
PVENT VentAdd(char *vname,char *size)
{
PVENT ve;																// New Entry
PVENT v2;																// Saved last entry
int sz;

	trim(vname);
	v2=LVBase[CurrentLVBase].last;										// Set up v2
	ve=malloc(sizeof(struct Varent));									// Allocate Var entry
	if(!ve) {return NULL;}												// Not Good at all
	memset(ve,0,sizeof(struct Varent));									// Clear it out
	LVBase[CurrentLVBase].count++;										// Add it in
	if(!LVBase[CurrentLVBase].last) {LVBase[CurrentLVBase].last=ve; LVBase[CurrentLVBase].first=ve;}					// First one please
	else {v2->next=ve; LVBase[CurrentLVBase].last=ve;}					// Now we are last
	sz=atoi(size);														// Get size of entry
	ve->len=sz;															// Update it
	ve->data=malloc(sz+1);												// Get storage for it
	memset(ve->data,0,sz+1);												// Clear it out
	if(!ve->data) {ve->len=0; return NULL;}								// Set length and return
	if(strlen(vname)<63) strcpy(ve->name,vname);						// If ok lenth
	else strncpy(ve->name,vname,63);									// else just copy 63
return ve;
}
//
// Add a Var Entry
//
int VentDel(char *vname)
{
PVENT scan=NULL;														// Vent entries
PVENT prev=NULL;

	scan=LVBase[CurrentLVBase].first;									// Load up staring point
	while(scan)
		{
		if(stricmp(vname,scan->name)==0)								// Find Var name
			{
			if(!prev) 
				{
				LVBase[CurrentLVBase].first=scan->next;					// First guy set new first
				VentFree(scan);											// Free this entry
				LVBase[CurrentLVBase].count--;							// Take one off
				if(!LVBase[CurrentLVBase].first) LVBase[CurrentLVBase].last=NULL;
				return 1;
				}
			else 
				{
				prev->next=scan->next;									// Set old prev to next
				if(LVBase[CurrentLVBase].last==scan) LVBase[CurrentLVBase].last=prev; // new last
				VentFree(scan);											// free this entry
				LVBase[CurrentLVBase].count--;							// Take one off
				return 1;
				}
			}
		prev=scan;														// Save this one
		scan=scan->next;												// Load up next
		}
return 1;
}
PVENT VentSet(char *var,int len,char *value)
{
PVENT ent=NULL;
char size[16];

	trim(var);
	sprintf(size,"%.15d",len);											// Make char size
	ent=FindVent(var);													// Dose the entry exists
	if(!ent) {VentAdd(var,size); ent=LVBase[CurrentLVBase].last;}		// No Create it
	if(!ent) {
		lmsg(MSG_ERR,"Unable to Allocate Storage for Variable [%s],",var);
		ExitProcess(8);
		}
	if(ent->len<len) {free(ent->data); ent->data=malloc(len+1);ent->len=len;}// make sure it will fit
	memset(ent->data,0,ent->len);											// Clear it out
	memcpy(ent->data,value,len);										// Set the data
return ent;
}
//
// Scan for a Var Entry
//
PVENT FindVent(char *vname)
{
PVENT scan;
	trim(vname);
	scan=LVBase[CurrentLVBase].first;													// Load up staring point
	while(scan)
		{
		if(stricmp(vname,scan->name)==0) return scan; 
		scan=scan->next;
		}
return NULL;
}
//
// Scan for a Var Data Address
//
PVENT FindVentAdrress(char *ptr)
{
PVENT scan;
	scan=LVBase[CurrentLVBase].first;													// Load up staring point
	while(scan)
		{
		if(ptr == scan->data) return scan; 
		scan=scan->next;
		}
return NULL;
}
//
// Scan for a Var by Name
//
PVENT FindVentName(char *name)
{
PVENT scan;
	scan=LVBase[CurrentLVBase].first;													// Load up staring point
	trim(name);
	while(scan)
		{
		if(stricmp(name,scan->name)==0) return scan; 
		scan=scan->next;
		}
return NULL;
}

//
// Reset a pointer to the proper Variable Data
//
char * LocalResolve(char *vname)
{
PVENT scan;
	scan=LVBase[CurrentLVBase].first;													// Load up staring point
	trim(vname);
	while(scan)
		{
		if(stricmp(vname,scan->name)==0) return scan->data; 
		scan=scan->next;
		}
return vname;
}