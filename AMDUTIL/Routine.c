#include <windows.h>
#include <winbase.h>
#include <winnetwk.h>
#include <shellAPI.h>
#include <shlguid.h>
#include <shlobj.h>
#include <math.h>
#include <mapi.h>
#include <cpl.h>
#include <ISGUIDS.H>
#include <direct.h>
#include <lzexpand.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "winsetup.h"
#include "schglobal.h"
#include "W32TRACE.h"
#include "parmblk.h"
#include "routine.h"
#include "amdnet.h"
#include "amdio.h"
#include "amdioutl.h"
#include "jdate.h"

#define MAXFILEBLOCKS 5
static FB FileBlocks[] = {
	{NULL,"CLOSED"},
	{NULL,"CLOSED"},
	{NULL,"CLOSED"},
	{NULL,"CLOSED"},
	{NULL,"CLOSED"},
	{NULL,"END"} };

static FUNCDEF allfunctions[] = {
	{"Local_Define",do_localdefine},
	{"Local_Drop",do_localdrop},
	{"Message",do_message},
	{"AskUser",do_askuser},
	{"Delay",do_delay},
	{"System",do_system},
	{"IF",do_if},
	{"END",do_end},
	{"ELSE",do_else},
	{"CALL",do_call},
	{"while",do_while},
	{"SaveVDF",do_savevdf},
	{"LoadVDF",do_loadvdf},
	{"SetVdfValue",do_setvalue},
	{"goto",do_goto},
	{"ADD",do_add},
	{"SUB",do_sub},
	{"MULT",do_mult},
	{"DIV",do_div},
	{"CreateLink",do_link},
	{"LogMsg",do_logmsg},
	{"FILE_EXISTS",do_fileexits},
	{"File_GetDate",do_filegetdate},
	{"File_SetDateTime",do_filesetdatetime},
	{"File_GetTime",do_filegettime},
	{"File_GetSize",do_filegetsize},
	{"File_GetAttr",do_filegetattr},
	{"File_SetAttr",do_filesetattr},
	{"File_Open",do_fileopen},
	{"File_Close",do_fileclose},
	{"File_Write",do_filewrite},
	{"File_Read",do_fileread},
	{"File_Remove",do_fileremove},
	{"Create_Dir",do_createdir},
	{"Remove_Dir",do_removedir},
	{"String_Trim",do_stringtrim},
	{"String_Copy",do_stringcopy},
	{"String_SUBSTR",do_stringsubstr},
	{"String_Find",do_stringfind},
	{"String_Len",do_stringlen},
	{"String_ConCat",do_stringcat},
	{"Create32",do_create32},
	{"Create16",do_create16},
	{"DispMsg",do_dispmsg},
	{"REG_ADD",do_regadd},
	{"REG_UPD",do_regupd},
	{"REG_DELETEVALUE",do_regdelv},
	{"REG_DELETEKEY",do_regdelk},
	{"REG_APPEND",do_regapp},
	{"REG_PREPEND",do_regpre},
	{"REG_REMOVESTRING",do_regrem},
	{"Reg_GetValue",do_regget},
	{"Ini_GetValue",do_iniget},
	{"Ini_SetValue",do_iniset},
	{"Get_SystemDir",do_getsysdir},
	{"Get_WindowsDir",do_getwindir},
	{"Get_ComputerName",do_computername},
	{"MAP_DRIVE",do_mapdrive},
	{"REMOVE_DRIVE",do_removedrive},
	{"Start_AMDSession",do_StartAMDSession},
	{"Stop_AMDSession",do_StopAMDSession},
	{"Send_AMDRequest",do_SendAMDRequest},
	{"AMD_ADD",do_amdadd},
	{"AMD_Open",do_amdopen},
	{"AMD_Unload",do_amdunload},
	{"AMD_Close",do_amdclose},
	{"AMD_Remove",do_amdremove},
	{"AMD_GetIndexOfName",do_amdgetindexofname},
	{"AMD_SORT",do_amdsort},
	{"AMD_GetSortedItem",do_getsorteditem},
	{"AMD_GetRecord",do_amdgetrecord},
	{"AMD_GetValue",do_amdgetvalue},
	{"AMD_SetValue",do_amdsetvalue},
	{"AMD_AddItem",do_amdadditem},
	{"AMD_DeleteItem",do_amddeleteitem},
	{"AMD_Flush",do_amdflush},
	{"Copy_File",do_copyfile},
	{"Copy_Compress",do_copycompress},
	{"Get_CurrentDir",do_getcurrentdir},
	{"Set_CurrentDir",do_setcurrentdir},
	{"BuildFileName",do_buildfilename},
	{"Get_TotalSpace",do_totaldisk},
	{"Get_FreeSpace",do_freedisk},
	{"Set_ExeTimeOut",do_exetimeout},
	{"Set_MaxLines",do_setmaxlines},
	{"Get_Date",do_getdate},
	{"Get_Time",do_gettime},
	{"Get_Julian",do_getjulian},
	{"Get_DateOffset",do_getdateoffset},
	{"File_GetDateTime",do_filegetdatetime},
	{"FINI",NULL},
	};
int maxlines=MAXLINES;											// Number of lines we'll allow to execute
int current_lines=0;											// Number of lines we have executed
//
// Do variable replace and if needed call routine to get value for parm
//
int scanparms(char *routine,PPARMBLK pb)
{
PARMBLK pb2;															// Parm block for 
char parmv2[MAXPARMS][ParmvSize];
int x=1,pos=0,rc;
char save_parm[ParmvSize];

	for(pos=0;pos<MAXPARMS;pos++){pb2.parms[pos]=&parmv2[pos][0];}	// Use our local buffers
	while(strlen(pb->parms[x])>0)									// while we have valid parms
		{
		if(stricmp(pb->parms[x],"RESULT")==0) {						// If Parm is Keywork RESULT copy over last exe result
			strcpy(pb->parms[x],pb->result);						// copy over last function result
			x++;													// increment the index
			continue;												// bounce to top of while
			}
		if(pb->parms[x][0]=='\"' && pb->parms[x][strlen(pb->parms[x])-1]=='\"')// Quoted string 
			{
			strcpy(pb->parms[x],&pb->parms[x][1]);					// Remove Double quotes
			pb->parms[x][strlen(pb->parms[x])-1]='\0';						// This will give us 
			x++; continue;											// Back to caller
			}
		Resolve(pb->parms[x]);  									// Resolve VDF Variable Now
	  	pb->parms[x]=LocalResolve(pb->parms[x]);					// Reset Parms pointer if needed
		if( (pos=findfirst(pb->parms[x],"("))>0) {
			if( (stricmp(routine,"while")==0) || (stricmp(routine,"if")==0) ) { 
				x++; continue;										// Allow thye while and if to process
				}
			memset(&parmv2,0,(ParmvSize*MAXPARMS));					// Clear it out
			strcpy(save_parm,pb->parms[x]);							// Save away parm field in case ( dose not mean function
			ParseRoutine(pb->parms[x],&pb2);						// Parse the line
			rc=isroutine(pb2.parms[0],&pb2);						// Execute function
			if(rc>-1) strcpy(pb->parms[x],pb2.result);				// Function found copy in result value
			else strcpy(pb->parms[x],save_parm);					// Function not found copy old parm back
			}
		x++;
		}
return x;
}
/********************************************************************************/
/**        Define the worker functions for setup of the routine calls          **/
/********************************************************************************/
int isroutine(char *routine,PPARMBLK pb)
{
int x=0,find=0;														// Index counter

	if(MaxLinesExe && LinesExecuted > MaxLinesExe) {				// We can do no more
		lmsg(MSG_ERR,"Max Lines Executed Reached [%d]",LinesExecuted);
		ExitProcess(8);
		}
	trim(routine);													// No trail blanks
	current_lines++;												// Count them up
	if(current_lines>maxlines) 
		{
		pb->curr_ps->fatal=1;										// Set Fatal Flag
		return 1;
		}
	scanparms(routine,pb);											// Resolve the parms
	x=0;
	while(allfunctions[x].function)
		{
		DispMsg();
		if(stricmp(routine,allfunctions[x].funcname)==0) 
			{ 
			lmsg(MSG_DEBUG,"  >>Call Function [%s] with [%s] [%s] [%s] [%s] [%s]",routine,pb->parms[1],pb->parms[2],pb->parms[3],pb->parms[4],pb->parms[5]);
			(*allfunctions[x].function) (pb);						// Call the Function found
			lmsg(MSG_DEBUG,"  >>Return from   [%s] was  [%s]",routine,pb->result); 
			LinesExecuted++;										// Keep track of this please
			if(pb->vartoset[0]!='\0')								// Variable to set for this guy
				{
				setresult(pb->vartoset,pb);							// Update the value
				memset(&pb->vartoset,0,_MAX_PATH);					// Reset after var update
				}
			return x;												// return offset found
			}	
		x++;
		}
return -1;
}
int RemoveBlanks(char *data)
{
int x=0;

	x=strlen(data);
	if(x<1) return 0;
	x--;
	while((data[x]<=' ') && x) {data[x]=0; x--;}
	while(data[0]==' ') strcpy(&data[0],&data[1]);
return x;
}

void setresult(char *var,PPARMBLK pb)
{
char b[_MAX_PATH];											// Base holder
char v[_MAX_PATH];											// Variable holder

	if(var[0]=='<')											// VDF Value Saved?
		{
		memset(b,0,sizeof(b));								// clear out base name
		memset(v,0,sizeof(v));								// Clear out variable
		strcpy(&var[0],&var[1]);							// Get rid of it
		var[strlen(var)-1]='\0';							// Get rid of back end one
		splitvar(var,b,v);									// Separate Var from base
		if(b[0]=='\0') strcpy(b,pb->curr_ps->sbase);		// Put in Base name
		setvalue(b,v,pb->result);							// save the value away
		return;												// Alldone
		}
	trim(pb->result);										// Clear out Blanks
	VentSet(var,strlen(pb->result),pb->result);				// Make sure its there
return;
}
int ParseRoutine(char *line,PPARMBLK pb)
{
int start=0,end=0,value=0,parmpos=0,x=0;

	if(strnicmp(line,"if ",3)==0 || strnicmp(line,"if(",3)==0) return(ParseIF(line,pb));
	if(strnicmp(line,"else ",5)==0 || strnicmp(line,"else(",5)==0) return(ParseElse(line,pb));
	if(strnicmp(line,"goto",4)==0) return(ParseGoto(line,pb));			// Parse Goto
	if(strnicmp(line,"while",5)==0) return (ParseWhile(line,pb));		// parse while statment
	pb->vartoset[0]='\0';												// Null out var to set
	start=findfirst(line,"(");											// Get first open brace
	if(start<0) {strcpy(pb->parms[parmpos],line); return 1;}			// Just a command
	strncpy(pb->parms[parmpos++],line,(start));							// Copy over routine name
	value=findfirst(pb->parms[0],"=");									// 0 Always the function name
	if(value>0) 
		{
		strncpy(pb->vartoset,pb->parms[0],value);						// copy up to = 
		strcpy(pb->parms[0],&pb->parms[0][value+1]);					// copy over the parms variable
		}
	end=findlast(line,")");											// Get last open brace
	line[end]='\0';													// chop it off
	start++; end--;													// go past the ( and before the )
	while(start < end)												// Now go through and get parms
		{
		if((x=findnextparm(&line[start],","))<0) break;				// All done						
		memset(pb->parms[parmpos],0,_MAX_PATH);						// clear it out
		strncpy(pb->parms[parmpos],&line[start],x);					// copy it in
		RemoveBlanks(pb->parms[parmpos++]);
		start+=(x+1);												// MOVE PAST IT
		}
	memset(pb->parms[parmpos],0,_MAX_PATH);							// Move in last parm
	strcpy(pb->parms[parmpos++],&line[start]);						// copy it in
	if(strnicmp(pb->parms[0],"IF",2)==0)							// Copy in Command or DO
		{
		memset(pb->parms[parmpos],0,_MAX_PATH);						// Move in last parm
		strcpy(pb->parms[parmpos++],&line[end+2]);					// copy it in
		}
	pb->numberofparms=parmpos;										// Set value
return 1;
}
//
// Parse Routine for the If statement
//
int ParseIF(char *line,PPARMBLK pb)
{
int start=0,end=0,parmpos=0,x=0;
char buf[_MAX_PATH];

	start=findfirst(line,"(");										// Get first open brace
	if(start<0) {lmsg(MSG_ERR,"Syntax error [%s]",line); return 0;} // Error no brace
	strncpy(pb->parms[parmpos++],line,start);						// Copy over the IF
	start++;
	x=findnextparm(&line[start],")");
	strncpy(pb->parms[parmpos++],&line[start],x);					// Copy over the IF
	end=start+x+1;
	strcpy(buf,&line[end]);											// copy rest of the line
	trim(buf);														// Remove blanks
	if(strnicmp(buf,"THEN",4)==0) 
		{
		strncpy(pb->parms[parmpos++],buf,4);						// Copy over the IF
		strcpy(buf,&buf[4]); trim(buf);								// Remove the "then"
		strcpy(pb->parms[parmpos++],buf);							// copy over command
		}
	else strcpy(pb->parms[parmpos++],buf);							// copy over command or "DO"
	pb->numberofparms=parmpos;										// Set value
return 1;
}
//
// Parse While statement 
//
int ParseWhile(char *line,PPARMBLK pb)
{
int start=0,end=0,parmpos=0,x=0;
char buf[_MAX_PATH];

	start=findfirst(line,"(");										// Get first open brace
	if(start<0) {lmsg(MSG_ERR,"Syntax error [%s]",line); return 0;} // Error no brace
	strncpy(pb->parms[parmpos++],line,start);						// Copy over the IF
	start++;
	x=findnextparm(&line[start],")");
	strncpy(pb->parms[parmpos++],&line[start],x);					// Copy over the IF
	end=start+x+1;
	strcpy(buf,&line[end]);											// copy rest of the line
	trim(buf);														// Remove blanks
	pb->numberofparms=parmpos;										// Set value
return 1;
}
// 
// Parse Routine for the Else Statment
//
int ParseElse(char *line,PPARMBLK pb)
{
int parmpos=0;

	strncpy(pb->parms[parmpos++],line,4);							// Copy over the IF
	strcpy(line,&line[4]); trim(line);								// Remove the else
	strcpy(pb->parms[parmpos++],line);								// Copy over the IF
	pb->numberofparms=parmpos;										// Set value
return 1;
}
int	ParseGoto(char *line,PPARMBLK pb)
{
int parmpos=0;
	strncpy(pb->parms[parmpos++],line,4);								// Copy over the call
	strcpy(line,&line[4]); trim(line);								// Remove the call
	strcpy(pb->parms[parmpos++],line);									// Copy over the Proc Name
	pb->numberofparms=parmpos;										// Set value
return 1;
}
//
// Find the first char that is in what
//
int findfirst(char *line,char *what)
{
int x=0;
	for(x=0;line[x]!='\0'; x++)										// Start at 0 and move forward
		{
		if(line[x]== what[0]) return x;								// If char found return position
		}	
return -1;
}
//
// Find the last char that is in what
//
int findlast(char *line,char *what)
{
int x=0;
	for(x=strlen(line);x>0; x--)									// Start at the last char
		{
		if(line[x]== what[0]) return x;								// If found return position
		}	
return -1;
}
//
// find a char starting at a given position
//
int findnextparm(char *line,char *what)
{
int x=0;
	for(x=0;line[x]>'\0'; x++)
		{
		if(line[x]=='(')  {x++; while(line[x]!=')'  && line[x]>'\0') x++;x++;}	// If a lower function call pass it up here  
		if(line[x]=='\"') {x++; while(line[x]!='\"' && line[x]>'\0') x++;x++;}	// If a quoted string pass it up
		if(line[x]== what[0]) return x;
		}
return -1;
}
//
// Find the END for this Block Start from a line scan while we are in the Proc level
// and the same line level
//
PLENT FindEnd(PLENT l,int pl,int ll)
{
PLENT work;

	work=l;																// Load up first line
	while(work)															// while we have lines
		{
		if(strnicmp(work->line,"END",3)!=0) {work=work->next; continue;}// Not an End line
		if(pl==work->proclevel && ll==work->level) return work;			// Found it
		work=work->next;												// Load up the next
		}
return NULL;															// Not found
}
//
// Find the ELSE for this Block Start from a line scan while we are in the Proc level
// and the same line level
//
PLENT FindElse(PLENT l,int pl,int ll)
{
PLENT work;

	work=l;																// Load up first line
	while(work)															// while we have lines
		{
		if(strnicmp(work->line,"END",3)==0) return NULL;				// Found an End First
		if(strnicmp(work->line,"ELSE",4)!=0) {work=work->next; continue;}   // Not an End line
		if(pl==work->proclevel && ll==work->level) return work;			// Found it
		work=work->next;												// Load up the next
		}
return NULL;															// Not found
}
/********************************************************************************/
/** All functions defined above will have there Routines coded here. All calls **/
/** get a PARMBLK structure so that they can get there parms and also give a   **/
/** Result back if they wish.												   **/
/********************************************************************************/
void do_message(PPARMBLK pb)
{
	lmsg(MSG_DEBUG,"Call to Message [%s]",pb->parms[1]);				// Debug Message
	MessageBox(NULL,pb->parms[1],"AMDUtil Message",MB_OK);
	strcpy(pb->result,GOOD);											// Put result into buffer
return;
}
void do_askuser(PPARMBLK pb)
{
int rsp=0;
	rsp=MessageBox(NULL,pb->parms[1],"AMDUTIL Message",MB_YESNO);
	if(rsp==IDYES) strcpy(pb->result,"YES");
	else strcpy(pb->result,"NO");
return;
}

void do_system(PPARMBLK pb)
{
	lmsg(MSG_DEBUG,"Call to System  [%s]",pb->parms[1]);				// Debug Message
	system(pb->parms[1]);												// Do the Sysytem Call
	strcpy(pb->result,GOOD);											// Put result into buffer
return;
}
void do_localdefine(PPARMBLK pb)
{
	lmsg(MSG_DEBUG,"Call to LocalDefine",pb->parms[1]);					// Debug Message
	VentAdd(pb->parms[1],pb->parms[2]);									// Call Vent add to add in variable
	strcpy(pb->result,GOOD);											// Put result into buffer
return;
}
void do_localdrop(PPARMBLK pb)
{
int x=1;
PVENT ent=NULL;

	lmsg(MSG_DEBUG,"Call to LocalDrop");								// Debug Message
	while( (strlen(pb->parms[x])>0) && (x<MAXPARMS) )	
		{
		ent=FindVentAdrress(pb->parms[x]);								// Find the entry
		if(ent) VentDel(ent->name);										// Call Vent add to add in variable
		ent=NULL;x++;
		}
	strcpy(pb->result,GOOD);											// Put result into buffer
return;
}

/********************************************************************************/
/*               Block of routines needed to do the IF/Else logic               */
/********************************************************************************/
void do_if(PPARMBLK pb)
{
char left[_MAX_PATH];													// left part of if
char right[_MAX_PATH];													// right part of if
char compare[3]="  ";													// compare string
PARMBLK pb2;															// Parm block for 
char parmv2[MAXPARMS][ParmvSize];
int x=0;																// Work Var
PLENT ent;																// Line entry
char *l,*r;																// Pointers for left and right

	for(x=0;x<MAXPARMS;x++){pb2.parms[x]=&parmv2[x][0];}				// Use our buffers
	memset(&parmv2,0,(ParmvSize*MAXPARMS));									// Clear it out
	memset(left,0,sizeof(left)); memset(right,0,sizeof(right));			// Clear out fields
	l=left;	r=right;													// Setup our pointers
	trim(pb->parms[2]);													// Clean up what to do
	pb2.curr_ps=pb->curr_ps;											// Current script pointer
	strcpy(pb2.curr_ps->sname,pb->curr_ps->sname);						// Copy over sname
	parseparm(pb->parms[1],left,right,compare);							// Get th values	
	trim(left); trim(right);
	if( (x=findfirst(left,"("))>0)										// left side is a function
		{
		ParseRoutine(left,&pb2);										// Parse the line
		isroutine(pb2.parms[0],&pb2);									// Execute function
		strcpy(left,pb2.result);										// copy over result
		memset(&parmv2,0,(ParmvSize*MAXPARMS));							// Clear it out
		}
	if( (x=findfirst(right,"("))>0)										// left side is a function
		{
		ParseRoutine(right,&pb2);										// Parse the line
		isroutine(pb2.parms[0],&pb2);									// Execute function
		strcpy(right,pb2.result);										// copy over result
		memset(&parmv2,0,(ParmvSize*MAXPARMS));							// Clear it out
		}
	if(stricmp(left,"RESULT")==0)  strcpy(left,pb->result);				// Is Keyword RESULT used
	if(stricmp(right,"RESULT")==0) strcpy(right,pb->result);			// Is Keyword RESULT used
  	l=LocalResolve(l);													// Reset Parms pointer if needed
	r=LocalResolve(r);
	trim(left); trim(right);
	//*****************************************************************
	// See if statement is true. if it is the check to see whats next *
	// If a "DO" is there just keep on executing at this level. If    *
	// you find a function EXEC it and save result.                   *
	//*****************************************************************
	if(istrue(l,r,compare))												// all done execute next 
		{
		if(stricmp(pb->parms[2],"DO")==0) {ProcessDo(pb);return;}		// Execute at this level
		if(stricmp(pb->parms[3],"DO")==0) {ProcessDo(pb);return;}		// Execute at this level
		ParseRoutine(pb->parms[3],&pb2);								// Parse the line
		isroutine(pb2.parms[0],&pb2);									// Execute function
		strcpy(pb->result,pb2.result);									// save result
		if(stricmp(pb2.parms[0],"goto")==0) {							// Handle the goto now
			pb->curr_ps->lnext=pb2.curr_ps->lnext;
			return;
			}
		ent=FindEnd(pb->curr_ps->lnext,pb->curr_ps->plevel,pb->curr_ps->level);
		if(ent) pb->curr_ps->lnext=ent;
		else {lmsg(MSG_ERR,"UnMatched End for IF [%s]",pb->curr_ps->lnext->line); pb->curr_ps->fatal=1;}
		return;
		}
	else																// See if there is an Else Clause
		{
		ent=FindElse(pb->curr_ps->lnext,pb->curr_ps->plevel,pb->curr_ps->level);
		if(ent) pb->curr_ps->lnext=ent;
		else
			{
			ent=FindEnd(pb->curr_ps->lnext,pb->curr_ps->plevel,pb->curr_ps->level);
			if(ent) pb->curr_ps->lnext=ent;
			else {lmsg(MSG_ERR,"UnMatched End for IF [%s]",pb->curr_ps->lnext->line); pb->curr_ps->fatal=1;}
			}
		return;
		}

return;
}
//
// Process the Else Statement
//
void do_else(PPARMBLK pb)
{
PARMBLK pb2;													// Parm block for 
char parmv2[MAXPARMS][ParmvSize];
int x;

	memset(&parmv2,0,(ParmvSize*MAXPARMS));							// Clear it out
	for(x=0;x<MAXPARMS;x++){pb2.parms[x]=parmv2[x];}			// Use our buffers
	pb2.curr_ps=pb->curr_ps;									// Current script pointer
	strcpy(pb2.curr_ps->sname,pb->curr_ps->sname);				// Copy over sname
	if(stricmp(pb->parms[1],"DO")==0) return;					// Next line already loaded
	ParseRoutine(pb->parms[1],&pb2);							// Parse the line
	isroutine(pb2.parms[0],&pb2);								// Execute function
	strcpy(pb->result,pb2.result);								// copy over result
return;
}
void do_end(PPARMBLK pb)
{
return;
}
int parseparm(char *stm,char *l,char *r,char *c)
{
char *p=NULL;
char table[6][3]={"==","!=","<",">","<=",">="};
int x=0;

	while(!p) {p=strstr(stm,table[x++]); if(x>5) break;}			// Find the compar value
    if(!p) return 0;												// OOOPs what can you do
	strncpy(c,p,2);													// load up the compare type
	*(p)='\0';														// Make a null
	strcpy(l,stm);													// put in left
	strcpy(r,p+2);													// put in right
	trim(l); trim(r); trim(c);										// remove blanks from back end
return 1;
}
int istrue(char *l,char *r,char *c)
{
int x=0;
	if(isnum(l) && isnum(r)) x=TestNum(l,r);					// DO Numeric compare
	else x=strcmp(l,r);											// compare the left and right side
	if(x==0)
		{
		if(strcmp(c,"==")==0) return 1;								// Yes this is TRUE
		if(strcmp(c,">=")==0) return 1;								// Yes this is True
		if(strcmp(c,"<=")==0) return 1;								// Yes this is True
		}
	if(x<0)
		{
		if(strcmp(c,"!=")==0) return 1;								// Yes this is True
		if(strcmp(c,"<")== 0) return 1;								// Yes this is true
		}
	if(x>0)
		{
		if(strcmp(c,"!=")==0) return 1;								// Yes this is True
		if(strcmp(c,">")== 0) return 1;								// Yes this is true
		}
return 0;
}
//
// Test two strings that are numeric
//
int TestNum(char *l,char *r)
{
DWORD nl=0,nr=0;

	sscanf(l,"%d",&nl);
	sscanf(r,"%d",&nr);
	if(nl < nr) return -1;
	if(nl == nr) return 0;
	if(nl > nr) return 1;
return 1;
}
//
// Determine if string is a numeric string
//
int isnum(char *c)
{
int x=0,y=0;

	x=strlen(c);
	while(isdigit(c[y])) {
		y++;
		if(x==y) return 1;
		}
return 0;
}
void ProcessDo(PPARMBLK pb)
{
PLENT nl;																// Line entry
char line[_MAX_PATH];													// Line hold
int routnum,i,level,plevel;												// local vars

	nl=pb->curr_ps->lnext;												// Point to line
	level=nl->level;													// Set this level
	plevel=nl->proclevel;												// Set Proc Level
	while(1)
		{
		nl=pb->curr_ps->lnext;											// Set up line
		if(nl->level>level){ProcessDo(pb); nl=pb->curr_ps->lnext;}		// If new Line Level
		if(nl->level<level) return;										// Bounce up
		if(nl->proclevel!=plevel) {UpdateLocalVars(pb->curr_ps);pb->curr_ps->lnext=pb->curr_ps->preturn; return;}// Outside of our proc
		memset(line,0,_MAX_PATH);										// Clear out buffer
		strcpy(line,nl->line);											// Copy it over
		pb->curr_ps->lnext=nl->next;									// Load up next line
		if(line[0]=='*'|| line[0]==':'|| (strncmp(line,"//",2)==0)) continue;	// Comment line or label
		for(i=0;i<MAXPARMS;i++){pb->parms[i]=parmv[i];}
		memset(&parmv,0,(ParmvSize*MAXPARMS));								// Clear it out
		ParseRoutine(line,pb);											// Parse the line
		if(stricmp(pb->parms[0],"ExitProcess")==0)
			{
			ExitProcess(atoi(pb->parms[1]));
			pb->curr_ps->fatal=(atoi(pb->parms[1]));
			pb->curr_ps->fatal=1;
			return;
			}
		if(stricmp(pb->parms[0],"END") ==0) return;						// Nothing more
		if(stricmp(pb->parms[0],"ELSE")==0)
			{
			nl=FindEnd(pb->curr_ps->lnext,pb->curr_ps->plevel,pb->curr_ps->level);
			if(nl) pb->curr_ps->lnext=nl;
			else {lmsg(MSG_ERR,"UnMatched End for IF [%s]",pb->curr_ps->lnext->line); pb->curr_ps->fatal=1;}
			return;
			}
		if( (routnum=isroutine(pb->parms[0],pb))<0) 
			{
			lmsg(MSG_ERR,"Routine not Supported [%s]",pb->parms[0]);	// Tell them about it
			continue;													// Just bypass for now
			}
		}

}
//
// Process the call STMT
//
void do_call(PPARMBLK pb)
{
PLENT ent;
int x=2;
PVENT lvent=NULL;														// Local var entry
PVENT lvent2=NULL;														// Second entry

	ent=FindProc(pb->curr_ps,pb->parms[1]);								// Get the proc line
	if(!ent) {lmsg(MSG_ERR,"Unable to find Proc [%s]",pb->parms[0]); return;}
	ent=ent->next;														// Skip proc line
	pb->curr_ps->preturn=pb->curr_ps->lnext;							// Set return as next line
	pb->curr_ps->lnext=ent->next;										// set to Proc line +1
	while(pb->parms[x][0]>' ')											// build new LVBase
		{
		lvent=FindVentAdrress(pb->parms[x]);							// See if its there
		if(!lvent) {x++; continue;}										// Not found next please
		CurrentLVBase++;												// Set next lVBase
		lvent2=VentSet(lvent->name,lvent->len,lvent->data);				// Make a copy
		lvent2->ref=lvent;												// Reference the copy
		x++;															// Next Parm to Check
		CurrentLVBase--;												// Reset back for Find
		}	
	CurrentLVBase++;													// Set new base entry
	ExecAtLevel(pb->curr_ps,ent->level,ent->proclevel,ent);
	return;
}
void do_while(PPARMBLK pb)
{
char left[_MAX_PATH];													// left part of if
char right[_MAX_PATH];													// right part of if
char compare[3]="  ";													// compare string
PARMBLK pb2;															// Parm block for 
char parmv2[MAXPARMS][ParmvSize];
int x=0;																// Work Var
PLENT ent;																// Line entry
PLENT ret_line;															// Line of the while stmt
char *l,*r;																// Pointers for left and right

	ret_line=pb->curr_ps->lnext->prev;									// Go back a line
	for(x=0;x<MAXPARMS;x++){pb2.parms[x]=&parmv2[x][0];}				// Use our buffers
	memset(&parmv2,0,(ParmvSize*MAXPARMS));									// Clear it out
	l=left;	r=right;													// Setup our pointers
	trim(pb->parms[2]);													// Clean up what to do
	pb2.curr_ps=pb->curr_ps;											// Current script pointer
	strcpy(pb2.curr_ps->sname,pb->curr_ps->sname);						// Copy over sname
	parseparm(pb->parms[1],left,right,compare);							// Get th values	
	if( (x=findfirst(left,"("))>0)										// left side is a function
		{
		ParseRoutine(left,&pb2);										// Parse the line
		isroutine(pb2.parms[0],&pb2);									// Execute function
		strcpy(left,pb2.result);										// copy over result
		}
	if( (x=findfirst(right,"("))>0)										// left side is a function
		{
		ParseRoutine(right,&pb2);										// Parse the line
		isroutine(pb2.parms[0],&pb2);									// Execute function
		strcpy(right,pb2.result);										// copy over result
		}
	if(stricmp(left,"RESULT")==0)  strcpy(left,pb->result);				// Is Keyword RESULT used
	if(stricmp(right,"RESULT")==0) strcpy(right,pb->result);			// Is Keyword RESULT used
  	l=LocalResolve(l);													// Reset Parms pointer if needed
	r=LocalResolve(r);
	//*****************************************************************
	// See if statement is true. if it is the check to see whats next *
	// If a "DO" is there just keep on executing at this level. If    *
	// you find a function EXEC it and save result.                   *
	//*****************************************************************
	if(istrue(l,r,compare))												// all done execute next 
		{
		ProcessDo(pb);													// Go run the lines
		pb->curr_ps->lnext=ret_line;									// Go back a line
		return;
		}
	else																// See if there is an Else Clause
		{
		ent=FindEnd(pb->curr_ps->lnext,pb->curr_ps->plevel,pb->curr_ps->level);
		if(ent) pb->curr_ps->lnext=ent;
		else {lmsg(MSG_ERR,"UnMatched End for IF [%s]",pb->curr_ps->lnext->line); pb->curr_ps->fatal=1;}
		}

return;
}
//
// Find the line that starts a procedure
//
PLENT FindProc(PSCR ps,char *name)
{
PLENT work;
char buf[_MAX_PATH];

	work=ps->lfirst;													// Load up first line
	while(work)															// while we have lines
		{
		if(strnicmp(work->line,"Proc:",5)!=0) {work=work->next; continue;} // Not a proc line
		strcpy(buf,&work->line[5]);										// Remove PROC:
		trim(buf);
		if(stricmp(buf,name)==0) return work;							// Found it
		work=work->next;												// Load up the next
		}
return NULL;															// Not found
}
void do_savevdf(PPARMBLK pb)
{
int rc=0;

	rc=SaveVbase(pb->curr_ps,pb->parms[1]);								// Save the Vbase
	sprintf(pb->result,"%d",rc);
return;
}
void do_loadvdf(PPARMBLK pb)
{
int rc=0;

	rc=LoadVbase(pb->curr_ps,pb->parms[1]);								// Save the Vbase
	sprintf(pb->result,"%d",rc);
return;
}
void do_setvalue(PPARMBLK pb)
{
int rc=0;
	if(pb->parms[1][0]=='\0') strcpy(pb->parms[1],pb->curr_ps->sbase);	// Set default to script.vdf
	setvalue((char *)pb->parms[1],(char *)pb->parms[2],(char *)pb->parms[3]);
	
return;
}
void do_goto(PPARMBLK pb)
{
PLENT work;
char n[_MAX_PATH];

	strcpy(n,pb->parms[1]);												// Copy over name
	work=pb->curr_ps->lfirst;											// Load up first line
	while(work)															// while we have lines
		{
		if(work->line[0]==':')
			if(stricmp(&work->line[1],n)==0){pb->curr_ps->lnext=work; return;}	
		work=work->next;												// Load up the next
		}
return;
}
void do_add(PPARMBLK pb)
{
int result=0;
int l=0,r=0;

	sscanf(pb->parms[1],"%d",&l);
	sscanf(pb->parms[2],"%d",&r);
	result=l+r;
	sprintf(pb->result,"%d",result);
}
void do_sub(PPARMBLK pb)
{
int result=0;
int l=0,r=0;

	sscanf(pb->parms[1],"%d",&l);
	sscanf(pb->parms[2],"%d",&r);
	result=l-r;
	sprintf(pb->result,"%d",result);
}
void do_mult(PPARMBLK pb)
{
int result=0;
int l=0,r=0;

	sscanf(pb->parms[1],"%d",&l);
	sscanf(pb->parms[2],"%d",&r);
	result=l*r;
	sprintf(pb->result,"%d",result);
}
void do_div(PPARMBLK pb)
{
div_t result;
int l=0,r=0;

	sscanf(pb->parms[1],"%d",&l);
	sscanf(pb->parms[2],"%d",&r);
	result=div(l,r);
	sprintf(pb->result,"%d.%d",result.quot,result.rem);
}
void do_link(PPARMBLK pb)
{
HRESULT hres=0;
IShellLink *psl;
IPersistFile *ppf;
WORD wsz[MAX_PATH];

	hres=CoCreateInstance(&CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,&IID_IShellLink,&psl);
	if(hres!=S_OK) {strcpy(pb->result,BAD); return;}
	psl->lpVtbl->SetPath(psl,pb->parms[2]);	
	psl->lpVtbl->SetDescription(psl,pb->parms[3]);
	psl->lpVtbl->SetWorkingDirectory(psl,pb->parms[4]);
	psl->lpVtbl->SetArguments(psl,pb->parms[5]);
	psl->lpVtbl->SetShowCmd(psl,SetShow(pb->parms[6]));
	hres=psl->lpVtbl->QueryInterface(psl,&IID_IPersistFile,&ppf);
	if(hres!=S_OK) {strcpy(pb->result,BAD); return;}
    MultiByteToWideChar( CP_ACP, 0, pb->parms[1], -1, wsz, MAX_PATH) ;
	hres=ppf->lpVtbl->Save(ppf,wsz,STGM_READWRITE);
	ppf->lpVtbl->Release(ppf);
	psl->lpVtbl->Release(psl);
	strcpy(pb->result,GOOD);
return;
}
void do_logmsg(PPARMBLK pb)
{
	lmsg(MSG_INFO,pb->parms[1],pb->parms[2],pb->parms[3],pb->parms[4],
		          pb->parms[5],pb->parms[6],pb->parms[7],pb->parms[8]);
	strcpy(pb->result,GOOD);
return;
}
//
// Start Of File System Routines
//
void do_fileexits(PPARMBLK pb)
{
	if( (_access( pb->parms[1], 0 )) != -1 ) strcpy(pb->result,GOOD);
	else strcpy(pb->result,BAD);
return;
}
void do_filegetdatetime(PPARMBLK pb)
{
char buffer[24];
int x=0;
	do_filegetdate(pb);							// Get the Files Date
	sprintf(buffer,"%s ",pb->result);			// Build the buffer
	do_filegettime(pb);							// Get the time
	strcat(buffer,pb->result);					// Add it in
	while(buffer[x++]>=' ') {					// Format like our AMD format
		if(buffer[x]=='/') buffer[x]=' ';
		if(buffer[x]==':') buffer[x]=' ';
		}
	strcpy(pb->result,buffer);
return;
}

void do_filegetdate(PPARMBLK pb)
{
WIN32_FIND_DATA fdata;								// Data from find file
HANDLE fh;											// Handle for call
SYSTEMTIME st;
FILETIME ft;

	if(!(fh=FindFirstFile(pb->parms[1],&fdata))) { strcpy(pb->result,BAD); return;}
    FileTimeToLocalFileTime(&fdata.ftLastWriteTime,&ft);
	FileTimeToSystemTime(&ft,&st);
	sprintf(pb->result,"%.4d/%.2d/%.2d",st.wYear,st.wMonth,st.wDay);
	FindClose(fh);
return;
}
void do_filesetdatetime(PPARMBLK pb)
{
HANDLE fle;
SYSTEMTIME st;
FILETIME ft;
FILETIME lt;

	if(!strlen(pb->parms[1]) || !strlen(pb->parms[2])) {
		strcpy(pb->result,BAD);
		return;
		}
	fle=CreateFile(pb->parms[1],GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	sscanf(pb->parms[2],"%d %d %d %d %d %d",&st.wYear,&st.wMonth,&st.wDay,&st.wHour,&st.wMinute,&st.wSecond);
	SystemTimeToFileTime(&st,&ft);
	LocalFileTimeToFileTime(&ft,&lt);
	if(!SetFileTime(fle,&lt,&lt,&lt)) {
		lmsg(MSG_ERR,"Set File Date/Time Failed %d",GetLastError());
		}
	CloseHandle(fle);
return;
}

void do_filegettime(PPARMBLK pb)
{
WIN32_FIND_DATA fdata;								// Data from find file
HANDLE fh;											// Handle for call
SYSTEMTIME st;
FILETIME ft;

	if(!(fh=FindFirstFile(pb->parms[1],&fdata))) { strcpy(pb->result,BAD); return;}
    FileTimeToLocalFileTime(&fdata.ftLastWriteTime,&ft);
	FileTimeToSystemTime(&ft,&st);
	sprintf(pb->result,"%.2d:%.2d:%.2d",st.wHour,st.wMinute,st.wSecond);
	FindClose(fh);
return;
}
void do_filegetsize(PPARMBLK pb)
{
WIN32_FIND_DATA fdata;								// Data from find file
HANDLE fh;											// Handle for call
long size;

	if(!(fh=FindFirstFile(pb->parms[1],&fdata))) { strcpy(pb->result,BAD); return;}
	size=fdata.nFileSizeHigh+fdata.nFileSizeLow;
	sprintf(pb->result,"%.12d",size);
	FindClose(fh);
return;
}
void do_filegetattr(PPARMBLK pb)
{
WIN32_FIND_DATA fdata;								// Data from find file
HANDLE fh;											// Handle for call

	memset(pb->result,0,10);						// Clear out field
	if(!(fh=FindFirstFile(pb->parms[1],&fdata))) { strcpy(pb->result,BAD); return;}
	FindClose(fh);
	if(fdata.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) strcat(pb->result,"A");
	if(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) strcat(pb->result,"D");
	if(fdata.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) strcat(pb->result,"H");
	if(fdata.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) strcat(pb->result,"N");
	if(fdata.dwFileAttributes & FILE_ATTRIBUTE_READONLY) strcat(pb->result,"R");
	if(fdata.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) strcat(pb->result,"S");
	if(fdata.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) strcat(pb->result,"T");
	if(fdata.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) strcat(pb->result,"C");
return;
}
void do_filesetattr(PPARMBLK pb)
{
WIN32_FIND_DATA fdata;								// Data from find file
HANDLE fh;											// Handle for call
DWORD fa;
int x=0,sel=0;

	strcpy(pb->result,"8");							// Make not selected
	if(!(fh=FindFirstFile(pb->parms[1],&fdata))) { strcpy(pb->result,BAD); return;}
	FindClose(fh);
	while(pb->parms[2][x]) {
		if(pb->parms[2][x]=='A') {fa=fa | FILE_ATTRIBUTE_ARCHIVE;	sel=1;}
		if(pb->parms[2][x]=='H') {fa=fa | FILE_ATTRIBUTE_HIDDEN;	sel=1;}
		if(pb->parms[2][x]=='N') {fa=fa | FILE_ATTRIBUTE_NORMAL;	sel=1;}
		if(pb->parms[2][x]=='R') {fa=fa | FILE_ATTRIBUTE_READONLY;	sel=1;}
		if(pb->parms[2][x]=='S') {fa=fa | FILE_ATTRIBUTE_SYSTEM;	sel=1;}
		if(pb->parms[2][x]=='T') {fa=fa | FILE_ATTRIBUTE_TEMPORARY;	sel=1;}
		if(pb->parms[2][x]=='C') {fa=fa | FILE_ATTRIBUTE_COMPRESSED;sel=1;}
		if(pb->parms[2][x]=='X') {fa=0; sel=1;}
		x++;
		}
	if(sel) sprintf(pb->result,"%d",SetFileAttributes(pb->parms[1],fa) );
return;
}

void do_fileopen(PPARMBLK pb)
{
int x=0;
	if ( (x=FindOpenFileBlock()) < 0) {sprintf(pb->result,BAD); return;}	// Get an open block
	FileBlocks[x].sfile=fopen(pb->parms[1],pb->parms[2]);
	if(FileBlocks[x].sfile==NULL) {sprintf(pb->result,BAD); return;}		// OPen failed
	if(pb->parms[3][0]==0) strcpy(pb->parms[3],"TEXT");						// Default is Text
	strcpy(FileBlocks[x].type,pb->parms[3]);								// Save away data type
	sprintf(pb->result,"%d",x);												// Place handle here
return;
}
int FindOpenFileBlock(void)
{
int x=0;
	while(x<MAXFILEBLOCKS) 
		{
		if(!FileBlocks[x].sfile) return x;
		x++;
		}
return -1;
}
void do_fileclose(PPARMBLK pb)
{
int x;
	x=atoi(pb->parms[1]);													// Get handle number
	if(x<0 || x>MAXFILEBLOCKS) {sprintf(pb->result,BAD); return; }			// Did'nt work
	if(!FileBlocks[x].sfile) {strcpy(FileBlocks[x].type,"CLOSED"); sprintf(pb->result,GOOD); return;}
	fclose(FileBlocks[x].sfile);
	strcpy(FileBlocks[x].type,"CLOSED");
	sprintf(pb->result,GOOD);
return;
}
void do_fileread(PPARMBLK pb)
{
int x=0,i=0,s=0;
PVENT ent;
char *c=NULL;

	x=atoi(pb->parms[1]);										// Get File block Offset
	if(x>MAXFILEBLOCKS)
		{
		strcpy(pb->result,BAD);
		lmsg(MSG_ERR,"Invalid File descriptor - File Not Opened");
		return;
		}
	s=atoi(pb->parms[3]);										// Size to get
	ent=FindVentAdrress(pb->parms[2]);							// Find the entry
	if(!ent)													// If not found 
		{
		VentAdd(pb->parms[2],pb->parms[3]);						// Add it in
		pb->parms[2]=LocalResolve(pb->parms[2]);				// Point Pb ant it
		ent=FindVentAdrress(pb->parms[2]);						// Load up ent with adress
		}
	if(!ent) {strcpy(pb->result,BAD);lmsg(MSG_ERR,"No Variable entry found for String_Copy"); return;}
	if(ent->len<s)												// Current define not OK
		{
		c=malloc(s);											// Can we get it
		if(!c) s=ent->len;										// No just get what you can
		else													// Yes Set it all up
		{free(ent->data); ent->data=c; ent->len=s; memset(c,0,s); pb->parms[2]=c;}
		}
	if(stricmp(FileBlocks[x].type,"BINARY")==0) i=fread(pb->parms[2],s,1,FileBlocks[x].sfile);
	else 
		{
		memset(pb->parms[2],0,s);								// Clear out buffer
		fgets(pb->parms[2],s,FileBlocks[x].sfile);				// Get the next string
		trim(pb->parms[2]);
		i=strlen(pb->parms[2]);									// length of data returned
		}
	sprintf(pb->result,"%d",i);									// Place number of bytes in result
return;
}
void do_filewrite(PPARMBLK pb)
{
int x=0,i=0,s=0;

	x=atoi(pb->parms[1]);										// Get File block Offset
	s=atoi(pb->parms[3]);										// Size to get
	if(stricmp(FileBlocks[x].type,"BINARY")==0) i=fwrite(pb->parms[2],(size_t)s,1,FileBlocks[x].sfile);
	else 
		{
		strcat(pb->parms[2],"\n");
		i=fputs(pb->parms[2],FileBlocks[x].sfile);				// Get the next string
		}
	sprintf(pb->result,"%d",i);									// Place number of bytes in result

return;
}
//
//   String Functions
//
void do_stringtrim(PPARMBLK pb)
{
int x=0;

	if( stricmp(pb->parms[2],"Right") ==0) 
		{ 
		x=(strlen(pb->parms[1]))-1;								// Find where to start
		while(pb->parms[1][x]==' ' && x>0) x--;					// Find the last blank
		pb->parms[1][++x]='\0';									// Put in the NULL
		sprintf(pb->result,"%d",strlen(pb->parms[1]));			// All ok
		}
	if( stricmp(pb->parms[2],"ALL") ==0)
		{ 
		trim(pb->parms[1]);										// Trim off the right 
		sprintf(pb->result,"%d",strlen(pb->parms[1]));			// All ok
		}
	if( stricmp(pb->parms[2],"Left") ==0)
		{ 
		while(pb->parms[1][x]<=' ' && pb->parms[1][x]>'\0') x++;// Remove Blanks at the left
		if(x>0) strcpy(pb->parms[1],&pb->parms[1][x]);			// Remove Blanks at left
		sprintf(pb->result,"%d",strlen(pb->parms[1]));			// All ok
		}
return;
}
void do_stringcopy(PPARMBLK pb)
{
int x=0,s=0;
PVENT ent;
char *c;
char expvar[2048];

	ent=FindVentAdrress(pb->parms[1]);							// Find the entry
	if(!ent)													// If not found 
		{
		VentAdd(pb->parms[1],"16");								// Add it in Default length of 16
		pb->parms[1]=LocalResolve(pb->parms[1]);				// Point Pb ant it
		ent=FindVentAdrress(pb->parms[1]);						// Load up ent with adress
		}
	if(!ent) {lmsg(MSG_ERR,"No Variable entry found for String_Copy"); return;}
	if(pb->parms[2][0]=='%') {
		ExpandEnvironmentStrings(pb->parms[2],expvar,2048);
		}
	else strcpy(expvar,pb->parms[2]);
	x=strlen(expvar);										// Get length of from field
	x++;														// Add in the NULL
	if(ent && ent->len<x)
		{
		c=malloc(x);											// Can we get it
		if(!c) x=ent->len;										// No just get what you can
		else													// Yes Set it all up
		{free(ent->data); ent->data=c; ent->len=x; memset(c,0,x); pb->parms[1]=c;}
		}
	strcpy(pb->parms[1],expvar);							// Copy over the data
	sprintf(pb->result,"%d",--x);								// Send back result						
return;
}
void do_stringlen(PPARMBLK pb)
{
	sprintf(pb->result,"%d",strlen(pb->parms[1]));				// Send back result
return;
}
void do_stringfind(PPARMBLK pb)
{
char *c;
int pos=0;
int start=0;
	if(strlen(pb->parms[3])>0) {
		start=atoi(pb->parms[3]);
		}
	c=strstr(&pb->parms[1][start],pb->parms[2]);				// Find the value in buffer
	if(!c) pos=0;												// Note found all ok
	else pos=c-pb->parms[1]+1;									// Position of data within string
	sprintf(pb->result,"%d",pos);								// Send back result						
return;
}
void do_stringsubstr(PPARMBLK pb)
{
int len=0,start;
PVENT ent;
char *c;

	start=atoi(pb->parms[3]);									// Starting pos
	if(start<0 || start>strlen(pb->parms[2])) {
		lmsg(MSG_ERR,"Invalid Start for Substring [%s]",pb->parms[1]);
		strcpy(pb->result,BAD);
		return;
		}
	len=atoi(pb->parms[4]);										// is length coded
	if(!len || len>strlen(pb->parms[2]) ||len<0) len=1+strlen(&pb->parms[2][start]);		// calac the length
	ent=FindVentAdrress(pb->parms[1]);							// Find the entry
	if(!ent)													// If not found 
		{
		VentAdd(pb->parms[1],"16");								// Add it in Default length of 16
		pb->parms[1]=LocalResolve(pb->parms[1]);				// Point Pb ant it
		ent=FindVentAdrress(pb->parms[1]);						// Load up ent with adress
		}
	if(!ent) {lmsg(MSG_ERR,"No Variable entry found for Substr"); return;}
	if(ent->len<len)
		{
		c=malloc(len);											// Can we get it
		if(!c) len=ent->len;									// No just get what you can
		else													// Yes Set it all up
		{free(ent->data); ent->data=c; ent->len=len; memset(c,0,len); pb->parms[1]=c;}
		}
	start--;													// Just copy the data
	memcpy(pb->parms[1],&pb->parms[2][start],len);				// Do the copy
	sprintf(pb->result,"%d",len);
return;
}
void do_stringcat(PPARMBLK pb)
{
int len1=0,len2=0;
PVENT ent;
char *c;

	len1=strlen(pb->parms[1]);									// Get recv field length
	len2=strlen(pb->parms[2]);									// Get length of data to cat
	len2++;														// Add in the NULL
	ent=FindVentAdrress(pb->parms[1]);							// Find the entry
	if(!ent)													// If not found 
		{
		VentAdd(pb->parms[1],"16");								// Add it in Default length of 16
		pb->parms[1]=LocalResolve(pb->parms[1]);				// Point Pb ant it
		ent=FindVentAdrress(pb->parms[1]);						// Load up ent with adress
		}
	if(!ent) {lmsg(MSG_ERR,"No Variable entry found for String_Concat"); return;}
	if(ent->len<(len1+len2))
		{
		strcpy(DataBuffer,pb->parms[1]);						// Save off the data
		c=malloc(len1+len2);									// Can we get it
		if(!c) len1=(ent->len-len2-1);							// No just get what you can
		else													// Yes Set it all up
		{free(ent->data); ent->data=c; ent->len=(len1+len2); memset(c,0,(len1+len2)); strcpy(c,DataBuffer);pb->parms[1]=c;}
		}
	strcat(pb->parms[1],pb->parms[2]);							//  do the concat
	sprintf(pb->result,"%d",strlen(pb->parms[1]));				// set result	
return;
}
void do_create32(PPARMBLK pb)
{
STARTUPINFO si = {0};
PROCESS_INFORMATION pi = {0};
DWORD rc;

      si.cb = sizeof(STARTUPINFO);
      si.lpReserved = NULL;
      si.lpReserved2 = NULL;
      si.cbReserved2 = 0;
      si.lpDesktop = NULL;
      si.dwFlags = 0;

	  if(pb->parms[2][0]>' ') si.wShowWindow=SetShow(pb->parms[2]);

	  if(pb->parms[4][0]==0) pb->parms[4]=NULL;				// Set to it Null if no data
      CreateProcess( NULL,
					 pb->parms[1],
                     NULL,
                     NULL,
                     FALSE,
                     NORMAL_PRIORITY_CLASS,
                     NULL,
                     pb->parms[4],
                     &si,
                     &pi );
	  DispMsg();
	  strcpy(pb->result,GOOD);
	  if(!pi.hProcess) {strcpy(pb->result,BAD); return;}
      if(stricmp(pb->parms[3],"wait")==0) {
			rc=WaitForSingleObject( pi.hProcess, ExeTimeout );
			if(rc==WAIT_TIMEOUT) strcpy(pb->result,"12");
			}
return;
}
void do_create16(PPARMBLK pb)
{
HMODULE h;
UINT inst;
	inst=WinExec(pb->parms[1],SW_SHOW);
	while( (h=GetModuleHandle(pb->parms[1])))
		{
		DispMsg();
		}
return;
}
WORD SetShow(char *var)
{
	if(stricmp(var,"SW_HIDE")==0) return SW_HIDE;
	if(stricmp(var,"SW_MAXIMIZE")==0) return SW_MAXIMIZE;
	if(stricmp(var,"SW_MINIMIZE")==0) return SW_MINIMIZE;
	if(stricmp(var,"SW_SHOW")==0) return SW_SHOW;
	if(stricmp(var,"SW_RESTORE")==0) return SW_RESTORE;	
	if(stricmp(var,"SW_SHOWDEFAULT")==0) return SW_SHOWDEFAULT;
	if(stricmp(var,"SW_SHOWMAXIMIZED")==0) return SW_SHOWMAXIMIZED; 	
	if(stricmp(var,"SW_SHOWMINIMIZED")==0) return SW_SHOWMINIMIZED;
	if(stricmp(var,"SW_SHOWMINNOACTIVE")==0) return SW_SHOWMINNOACTIVE;
	if(stricmp(var,"SW_SHOWNA")==0) return SW_SHOWNA;
	if(stricmp(var,"SW_SHOWNORMAL")==0) return SW_SHOWNORMAL;
	if(stricmp(var,"SW_SHOWNOACTIVATE")==0) return SW_SHOWNOACTIVATE;
return SW_SHOW;
}
void do_dispmsg(PPARMBLK pb)
{
	DispMsg();
}
void do_regadd(PPARMBLK pb)
{
	strcpy(COper,"ADD");									// Set add Function
	strncpy(CHive,pb->parms[1],REGHIVE);					// Copy in the Hive
	strncpy(CKey,pb->parms[2],REGKEY);						// Move in Key name
	strncpy(CValue,pb->parms[3],REGVALUENAME);				// Update Value name
	strncpy(CDataType,pb->parms[4],REGDATATYPE);			// Copy in the data type
	strncpy(CData,pb->parms[5],REGDATA);					// Copy in the data
	if (ProcessAction()) strcpy(pb->result,GOOD);			// Do the function
	else strcpy(pb->result,BAD);							// Oh Noooooo
return;
}
void do_regupd(PPARMBLK pb)
{
	strcpy(COper,"ADD");									// Set add Function
	strncpy(CHive,pb->parms[1],REGHIVE);					// Copy in the Hive
	strncpy(CKey,pb->parms[2],REGKEY);						// Move in Key name
	strncpy(CValue,pb->parms[3],REGVALUENAME);				// Update Value name
	strncpy(CDataType,pb->parms[4],REGDATATYPE);			// Copy in the data type
	strncpy(CData,pb->parms[5],REGDATA);					// Copy in the data
	if (ProcessAction()) strcpy(pb->result,GOOD);			// Do the function
	else strcpy(pb->result,BAD);							// Oh Noooooo
return;

}
void do_regdelv(PPARMBLK pb)
{
	strcpy(COper,"Deletev");								// Set add Function
	strncpy(CHive,pb->parms[1],REGHIVE);					// Copy in the Hive
	strncpy(CKey,pb->parms[2],REGKEY);						// Move in Key name
	strncpy(CValue,pb->parms[3],REGVALUENAME);				// Update Value name
	if (ProcessAction()) strcpy(pb->result,GOOD);			// Do the function
	else strcpy(pb->result,BAD);							// Oh Noooooo
return;
}
void do_regdelk(PPARMBLK pb)
{
	strcpy(COper,"Deletek");								// Set add Function
	strncpy(CHive,pb->parms[1],REGHIVE);					// Copy in the Hive
	strncpy(CKey,pb->parms[2],REGKEY);						// Move in Key name
	if (ProcessAction()) strcpy(pb->result,GOOD);			// Do the function
	else strcpy(pb->result,BAD);							// Oh Noooooo
return;
}
void do_regapp(PPARMBLK pb)
{
	strcpy(COper,"Append");									// Set add Function
	strncpy(CHive,pb->parms[1],REGHIVE);					// Copy in the Hive
	strncpy(CKey,pb->parms[2],REGKEY);						// Move in Key name
	strncpy(CValue,pb->parms[3],REGVALUENAME);				// Update Value name
	strncpy(CDataType,pb->parms[4],REGDATATYPE);			// Copy in the data type
	strncpy(CData,pb->parms[5],REGDATA);					// Copy in the data
	if (ProcessAction()) strcpy(pb->result,GOOD);			// Do the function
	else strcpy(pb->result,BAD);							// Oh Noooooo
return;
}
void do_regpre(PPARMBLK pb)
{
	strcpy(COper,"PrePend");								// Set add Function
	strncpy(CHive,pb->parms[1],REGHIVE);					// Copy in the Hive
	strncpy(CKey,pb->parms[2],REGKEY);						// Move in Key name
	strncpy(CValue,pb->parms[3],REGVALUENAME);				// Update Value name
	strncpy(CDataType,pb->parms[4],REGDATATYPE);			// Copy in the data type
	strncpy(CData,pb->parms[5],REGDATA);					// Copy in the data
	if (ProcessAction()) strcpy(pb->result,GOOD);			// Do the function
	else strcpy(pb->result,BAD);							// Oh Noooooo
return;
}
void do_regrem(PPARMBLK pb)
{
	strcpy(COper,"RemoveS");								// Set add Function
	strncpy(CHive,pb->parms[1],REGHIVE);					// Copy in the Hive
	strncpy(CKey,pb->parms[2],REGKEY);						// Move in Key name
	strncpy(CValue,pb->parms[3],REGVALUENAME);				// Update Value name
	strncpy(CDataType,pb->parms[4],REGDATATYPE);			// Copy in the data type
	strncpy(CData,pb->parms[5],REGDATA);					// Copy in the data
	if (ProcessAction()) strcpy(pb->result,GOOD);			// Do the function
	else strcpy(pb->result,BAD);							// Oh Noooooo
return;
}
void do_regget(PPARMBLK pb)
{
int len=0;
PVENT ent=NULL;
char *c=NULL;

	strcpy(COper,"GetV");									// Set add Function
	strncpy(CHive,pb->parms[1],REGHIVE);					// Copy in the Hive
	strncpy(CKey,pb->parms[2],REGKEY);						// Move in Key name
	strncpy(CValue,pb->parms[3],REGVALUENAME);				// Update Value name
	if ( (len=ProcessAction()) <= 0) strcpy(pb->result,GOOD);
	strcpy(pb->result,BAD);									// Oh Noooooo
	ent=FindVentAdrress(pb->parms[4]);						// Find the entry
	if(!ent)												// If not found 
		{
		sprintf(pb->parms[5],"%d",len);						// Make it char
		VentAdd(pb->parms[4],pb->parms[5]);					// Add it in Default length of 16
		pb->parms[4]=LocalResolve(pb->parms[4]);			// Point Pb ant it
		ent=FindVentAdrress(pb->parms[4]);					// Load up ent with adress
		}
	if(!ent) {lmsg(MSG_ERR,"No Variable entry found for String_Concat"); return;}
	if(len==0) len=ent->len;								// Set to existing length 
	if(ent->len<len)
		{
		c=malloc(len);										// Can we get it
		if(!c) len=ent->len;								// No just get what you can
		else												// Yes Set it all up
		{free(ent->data); ent->data=c; ent->len=len; memset(c,0,len); pb->parms[4]=c;}
		}
	memcpy(pb->parms[4],DataBuffer,len);					// Move in the Data
return;
}
void do_iniget(PPARMBLK pb)
{
int len=0;
PVENT ent=NULL;
char *c=NULL;
DWORD dlen=0;

	memset(DataBuffer,0,sizeof(DataBuffer));
	dlen=GetPrivateProfileString(pb->parms[1],pb->parms[2],"",DataBuffer,DATABUFSIZE,pb->parms[3]);
	if ( dlen ) strcpy(pb->result,GOOD);
	else strcpy(pb->result,BAD);							// Oh Noooooo
	len=(int)dlen;
	ent=FindVentAdrress(pb->parms[4]);						// Find the entry
	if(!ent)												// If not found 
		{
		sprintf(pb->parms[5],"%d",dlen+1);					// Make it char
		VentAdd(pb->parms[4],pb->parms[5]);					// Add it in Default length of 16
		pb->parms[4]=LocalResolve(pb->parms[4]);			// Point Pb ant it
		ent=FindVentAdrress(pb->parms[4]);					// Load up ent with adress
		}
	if(!ent) {lmsg(MSG_ERR,"No Variable entry found for IniGet"); return;}
	if(ent->len<len)
		{
		c=malloc(len);										// Can we get it
		if(!c) len=ent->len;								// No just get what you can
		else												// Yes Set it all up
		{free(ent->data); ent->data=c; ent->len=len; memset(c,0,len); pb->parms[4]=c;}
		}
	memcpy(pb->parms[4],DataBuffer,len);					// Move in the Data
return;
}
void do_iniset(PPARMBLK pb)
{
int len=0;
PVENT ent=NULL;
char *c=NULL;
DWORD dlen=0;

	if(WritePrivateProfileString(pb->parms[1],pb->parms[2],pb->parms[4],pb->parms[3]))
		strcpy(pb->result,GOOD);
	else strcpy(pb->result,BAD);							// Oh Noooooo
	WritePrivateProfileString(NULL,NULL,NULL,pb->parms[3]);
return;
}
void do_getsysdir(PPARMBLK pb)
{
	GetSystemDirectory(pb->result,_MAX_PATH);
	trim(pb->result); strcat(pb->result,"\\");
return;
}
void do_getwindir(PPARMBLK pb)
{
	GetWindowsDirectory(pb->result,_MAX_PATH);
	trim(pb->result); strcat(pb->result,"\\");
return;
}
void do_mapdrive(PPARMBLK pb)
{
DWORD stat=0;

	if(pb->numberofparms == 4) 					// Password Provided
		stat=WNetAddConnection(pb->parms[1],pb->parms[3],pb->parms[2]);
	else stat=WNetAddConnection(pb->parms[1],NULL,pb->parms[2]);
	if (stat) strcpy(pb->result,GOOD);				// Do the function
	else strcpy(pb->result,BAD);					// Oh Noooooo

return;
}
void do_removedrive(PPARMBLK pb)
{
DWORD stat=0;
	stat=WNetCancelConnection(pb->parms[1],TRUE);
	if (stat) strcpy(pb->result,GOOD);			// Do the function
	else strcpy(pb->result,BAD);							// Oh Noooooo

return;
}
void do_copyfile(PPARMBLK pb)
{
	if(CopyFile(pb->parms[1],pb->parms[2],FALSE)) strcpy(pb->result,GOOD);
	else strcpy(pb->result,BAD);
return;
}
void do_copycompress(PPARMBLK pb)
{
OFSTRUCT of1,of2;
INT	f1,f2,f3;
long rc;

	f1=LZOpenFile(pb->parms[1],&of1,OF_READ | OF_SHARE_DENY_NONE);
	f2=LZOpenFile(pb->parms[2],&of2,OF_CREATE);
	if(f1<0 || f2<0) {
		lmsg(MSG_ERR,"Open Failure for LZCOPY I[%d] O[%d]",pb->parms[1],pb->parms[2]);
		strcpy(pb->result,BAD);
		return;
		}
	rc=LZCopy(f1,f2);
	LZClose(f1);
	LZClose(f2);
	sprintf(pb->result,"%d",rc);
return;
}

void do_fileremove(PPARMBLK pb)
{
	SetFileAttributes((LPCTSTR)pb->parms[1],(DWORD)0);
	if(!remove(pb->parms[1]) ) strcpy(pb->result,GOOD);
	else strcpy(pb->result,BAD);
return;
}
void do_delay(PPARMBLK pb)
{
int sec=0,x=0;

	sscanf(pb->parms[1],"%d",&sec);
	sec=sec*2;
	if (sec>120) sec=10;
	for(x=0;x<sec;x++) {SleepEx(500,FALSE); DispMsg();}
	strcpy(pb->result,GOOD);
return;
}
void do_createdir(PPARMBLK pb)
{
char buffer[_MAX_PATH+1];
char *c,*s;
	buffer[0]='\0';
	if(strlen(pb->parms[1])==0) {strcpy(pb->result,BAD); return;}	
	if(pb->parms[1][strlen(pb->parms[1])-1]!='\\') strcat(pb->parms[1],"\\");
	if( (_access(pb->parms[1], 0 )) != -1 ) return;							
	c=strstr(pb->parms[1],"\\"); s=pb->parms[1]; c++;									
	while(c) 
		{
		c=strstr(c,"\\");											
		if(!c) break;												
		*(c)='\0';													
		strcat(buffer,s);											
		mkdir(buffer);												
		strcat(buffer,"\\");										
		c=c+1; s=c;													
		}
return;
}
void do_removedir(PPARMBLK pb)
{
	if(RemoveDirectory(pb->parms[1])) strcpy(pb->result,GOOD);
	else strcpy(pb->result,BAD);
return;
}
void do_computername(PPARMBLK pb)
{
char buffer[_MAX_PATH+1];
DWORD len=_MAX_PATH;

	GetComputerName(buffer,&len);
	strcpy(pb->result,buffer);

return;
}
void do_StartAMDSession(PPARMBLK pb)
{
	if(AMDSessionStarted) {
		lmsg(MSG_ERR,"Only One AMD Session Allowed per Script");
		strcpy(pb->result,BAD);
		return;
		}
	if(StartTcpSession(pb->parms[1])) {strcpy(pb->result,GOOD); AMDSessionStarted=1;}
	else {strcpy(pb->result,BAD); AMDSessionStarted=0;}
return;
}

void do_StopAMDSession(PPARMBLK pb)
{
	StopClient(s);
	AMDSessionStarted=0;
return;
}

void do_SendAMDRequest(PPARMBLK pb)
{
	if(strlen(pb->parms[3]) == 0) pb->parms[3]=NULL;
	if(strlen(pb->parms[4]) == 0) pb->parms[4]=NULL;
	if(SendAMDRequest(pb->parms[1],pb->parms[2],pb->parms[3],pb->parms[4])) strcpy(pb->result,ib->recbuf);
	else strcpy(pb->result,BAD);
return;
}
//
// Add a File Name into the AMD environment
//
void do_amdadd(PPARMBLK pb)
{
int handle;
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {InitAMD(); do_init();}							// do it the first time
	if( (handle=GetNextBlk()) < 0) 
		{
		lmsg(MSG_INFO,"Unable to get a handle for AMD request");
		strcpy(pb->result,BAD); 
		return;
		}	
	Master[handle].pv=AddAMD(pb->parms[1]);							// add it in please
	if(!Master[handle].pv) {
		lmsg(MSG_INFO,"Unable to Add AMD File Request");			// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	sprintf(pb->result,"%d",handle);								// Give caller the handle
	return;
}
void do_amdopen(PPARMBLK pb)
{
int handle;

	strcpy(pb->result,GOOD);
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	if(!OpenAMD(Master[handle].pv)) {
		lmsg(MSG_INFO,"Unable to Open AMD File [%s]",pb->parms[1]);					// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
return;
}
void do_amdunload(PPARMBLK pb)
{
int handle;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	if(!UnloadAMD(Master[handle].pv)) {
		lmsg(MSG_INFO,"Unable to Unload AMD File [%s]",pb->parms[1]);				// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	Master[handle].li=Master[handle].pv->Fbase;										// Set the line items
	BuildItemList(Master[handle].pv,&Master[handle].pv->Curr,Master[handle].li);	// build fields for this guy
return;
}
void do_amdflush(PPARMBLK pb)
{
int handle;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	if(!FlushAMD(Master[handle].pv)) {
		lmsg(MSG_INFO,"Unable to Flush AMD File [%s]",pb->parms[1]);				// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
return;
}
void do_amdclose(PPARMBLK pb)
{
int handle;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	if(!CloseAMD(Master[handle].pv)) {
		lmsg(MSG_INFO,"Unable to Close AMD File [%s]",pb->parms[1]);					// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
return;
}
void do_amdremove(PPARMBLK pb)
{
int handle;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	if(!RemoveAMD(Master[handle].pv)) {
		lmsg(MSG_INFO,"Unable to Remove AMD File [%s]",pb->parms[1]);				// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	memset(&Master[handle],0,sizeof(AMDBLOCK));
return;
}
void do_amdgetindexofname(PPARMBLK pb)
{
int handle,index;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	if( (index=GetIndexOfName(Master[handle].pv,pb->parms[2]))<0) {					// you make the call
		lmsg(MSG_INFO,"Unable to Find Field in AMD File [%s] [%s]",pb->parms[1],pb->parms[2]);
		strcpy(pb->result,BAD); 
		return;
		}
	sprintf(pb->result,"%d",index);
return;
}
void do_amdsort(PPARMBLK pb)
{
int handle;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	if(!Master[handle].psort) 
		Master[handle].psort=malloc((int)(Master[handle].pv->records*sizeof(char *)));	// Allocate it please
	SortAMD(Master[handle].pv,pb->parms[2],Master[handle].psort);
return;
}
void do_getsorteditem(PPARMBLK pb)
{
int handle,index;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	index=atoi(pb->parms[2]);
	if(index<0 || index>(Master[handle].pv->records-1)) {
		lmsg(MSG_INFO,"Invalid Record Number AMD File Request [%s]",pb->parms[2]);	// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	GetSortedItemAMD(Master[handle].pv,Master[handle].psort,index);
	SetMasterLi(handle,Master[handle].psort,index);
return;
}
void do_amdgetrecord(PPARMBLK pb)
{
int handle,recnum;
	strcpy(pb->result,BAD);
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	recnum=atoi(pb->parms[2]);
	if(recnum<0 || recnum>(int)Master[handle].pv->records) {
		lmsg(MSG_INFO,"Invalid record Number for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	LocateRecord(handle,recnum);
	if(Master[handle].li) strcpy(pb->result,GOOD);
return;
}
void do_amdgetvalue(PPARMBLK pb)
{
int handle,index;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	index=atoi(pb->parms[2]);														// Get the index please
	if(index<0 || index >(int) Master[handle].pv->inums) {
		lmsg(MSG_INFO,"Invalid Index for AMD File [%s]",pb->parms[2]);				// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	strncpy(pb->result,Master[handle].pv->Curr.items[index],_MAX_PATH);				// Copy it please
return;
}
void do_amdadditem(PPARMBLK pb)
{
int handle;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	if(AddItemAMD(Master[handle].pv,pb->parms[2])) {
		lmsg(MSG_INFO,"Item Add Failed for AMD File [%s]",pb->parms[2]);			// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
return;
}
void do_amddeleteitem(PPARMBLK pb)
{
int handle;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	if(DeleteItemAMD(Master[handle].pv,pb->parms[2])) {
		lmsg(MSG_INFO,"Item Delete Failed for AMD File [%s]",pb->parms[2]);			// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
return;
}
void do_amdsetvalue(PPARMBLK pb)
{
int handle;

	strcpy(pb->result,GOOD); 
	if(!strlen(pb->parms[1])) {strcpy(pb->result,BAD); return;}
	if(!InitDone) {strcpy(pb->result,BAD); return;}
	if( (handle=TestHandle(pb->parms[1])) < 0) {
		lmsg(MSG_INFO,"Invalid Handle for AMD File Request [%s]",pb->parms[1]);		// Tell them
		strcpy(pb->result,BAD); 
		return;
		}
	//											  Field	       Data
	if(!UpdateAMD(Master[handle].pv,Master[handle].li,pb->parms[2],pb->parms[3])) {
		lmsg(MSG_INFO,"Update failed for AMD File Request [%s] [%s]",pb->parms[2],pb->parms[3]);
		strcpy(pb->result,BAD); 
		return;
		}
return;
}
//
// Scan for a Free Block
//
int GetNextBlk(void)
{
int x;
	for(x=0;x<MAXBLK;x++) {
		if(!Master[x].inuse) {
			Master[x].inuse=1;
			return x;
			}
		}
return -1;
}
//
// Convert and Test the Char handle to an int
//
int TestHandle(char *han)
{
int x;
	x=atoi(han);
	if( (x<0) || (x>MAXBLK) ) return -1;			// out of range
	if(!Master[x].inuse) return -1;					// Block not in use
return x;
}
//
// Init the AMD environment
//
void do_init(void)
{
int x;
	for(x=0;x<MAXBLK;x++) {
		memset(&Master[x],0,sizeof(AMDBLOCK));
	}
	InitDone=1;
}
// Make PV.Current record the recnum
//
int LocateRecord(int handle,int recnum)
{
struct LineEntry *li;
int i=0;					
	li=Master[handle].pv->Fbase;							// Start of the list
	while(li)
		{
		if(i==recnum) break;								// Found it
		i++; li=li->next;									// Next one
		}
	if(li) BuildItemList(Master[handle].pv,&Master[handle].pv->Curr,li);	// Make it current
	Master[handle].li=li;									// Set it now please
return 0;													// Not Found
}
int SetMasterLi(int handle,struct LineEntry *from[],int num)
{
struct LineEntry *li;
	li=from[num];
	Master[handle].li=li;
return 1;
}
void do_getcurrentdir(PPARMBLK pb)
{
DWORD len=_MAX_PATH;

	GetCurrentDirectory(len,pb->result);
return;
}
void do_setcurrentdir(PPARMBLK pb)
{
	if(SetCurrentDirectory(pb->parms[1])) strcpy(pb->result,GOOD);
	else strcpy(pb->result,BAD);
return;
}
void do_buildfilename(PPARMBLK pb)
{
char work1[_MAX_PATH+1];
char work2[_MAX_PATH+1];

	strcpy(work1,pb->parms[1]); trim(work1);
	strcpy(work2,pb->parms[2]);	trim(work2);
	if(work1[strlen(work1)-1]!='\\') strcat(work1,"\\");	// put the slash in
	if(work2[0]=='\\') strcpy(&work2[0],&work2[1]);			// take the slah out
	sprintf(pb->result,"%s%s",work1,work2);
return;
}
void do_freedisk(PPARMBLK pb)
{
    DWORD 	dwSectorsPerCluster, dwTotalClusters ;
    DWORD   dwBytesPerSector, 	dwFreeClusters ;
	DWORD	dwfree=0;

   if(GetDiskFreeSpace(pb->parms[1],&dwSectorsPerCluster,&dwBytesPerSector,&dwFreeClusters,&dwTotalClusters))
			{
			dwfree=(dwBytesPerSector*dwSectorsPerCluster)*dwFreeClusters;
			}	
	sprintf(pb->result,"%lu",dwfree);
return;
}
void do_totaldisk(PPARMBLK pb)
{
    DWORD 	dwSectorsPerCluster, dwTotalClusters ;
    DWORD   dwBytesPerSector, 	dwFreeClusters ;
	DWORD	dwtotal=0;

   if(GetDiskFreeSpace(pb->parms[1],&dwSectorsPerCluster,&dwBytesPerSector,&dwFreeClusters,&dwTotalClusters))
			{
			dwtotal=(dwBytesPerSector*dwSectorsPerCluster)*dwTotalClusters;
			}	
	sprintf(pb->result,"%lu",dwtotal);
return;
}
void do_exetimeout(PPARMBLK pb)
{
DWORD timeout;
	timeout=atol(pb->parms[1]);					// Get the minutes value
	timeout=timeout*1000;						// Make milliseconds
	ExeTimeout=timeout*60;						// Turn into minutes
return;
}
void do_setmaxlines(PPARMBLK pb)
{
	MaxLinesExe=atol(pb->parms[1]);				// Get the minutes value
return;
}

void do_getdateoffset(PPARMBLK pb)
{
float jul;
float offset;

	jul=JulianOf(pb->parms[1]);
	offset=atof(pb->parms[2]);
	jul=jul+offset;
	DateOfJulian(jul,pb->result);
}
void do_getdate(PPARMBLK pb)
{
SYSTEMTIME tme;
	GetLocalTime(&tme);
	sprintf(pb->result,"%.4d/%.2d/%.2d",tme.wYear,tme.wMonth,tme.wDay);
return;
}
void do_gettime(PPARMBLK pb)
{
SYSTEMTIME tme;
	GetLocalTime(&tme);
	sprintf(pb->result,"%.2d:%.2d:%.2d",tme.wHour,tme.wMinute,tme.wSecond);
return;
}
void do_getjulian (PPARMBLK pb)
{
float jul;

	jul=JulianOf(pb->parms[1]);
	sprintf(pb->result,"%f",jul);
}
