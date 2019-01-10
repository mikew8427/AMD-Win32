#include "windows.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "W32TRACE.h"
#include "parmblk.h"
/************************************************/
/**				Global Variables               **/
/************************************************/
int     CurrentSection=0;             /* Used to build Section	*/
char    COper[REGOPER];               /* Operation to perform   */
char    CKey[REGKEY];                 /* Key name				*/
char    CValue[REGVALUENAME];         /* Value name for Key     */
char    CDataType[REGDATATYPE];       /* Data type defined      */
DWORD	CDataLength;				  /* Length of data defined */
char    CData[REGDATA];               /* Buffer for Data        */
char    CHive[REGHIVE];               /* Hive Name              */
char	CObj[EDMSIZE+1];			  /* Edm Object Name		*/
char	CVar[EDMSIZE+1];			  /* EDM Var name			*/
char	CHeap[EDMSIZE+1];			  /* EDM HEap Number		*/
HKEY    CurrentHive;                  /* HKEY For hive          */
char    DataBuffer[DATABUFSIZE]; 
/************************************************/
/* Function Prototypes                          */
/************************************************/
int ProcessAction();
int CreateNewKey();
int DeleteValue();
int DeleteKey();
int AppendValue();
int PrependValue();
int GetValue();
int RemoveString();
DWORD LengthOf(DWORD w);
HKEY SetHkey(char *hive);
int FormatData(char *);
DWORD SetValueType(char *val);
int formatstring(char *in);
int	formatbinary(char *in);

//
// Convert Key name to Defined Named
//
HKEY SetHkey(char *hive)
{
	if(stricmp(hive,"HKEY_CLASSES_ROOT")==0) return HKEY_CLASSES_ROOT;
	if(stricmp(hive,"HKEY_CURRENT_USER")==0) return HKEY_CURRENT_USER;
	if(stricmp(hive,"HKEY_LOCAL_MACHINE")==0) return HKEY_LOCAL_MACHINE;
	if(stricmp(hive,"HKEY_USERS")==0) return HKEY_USERS;
	if(stricmp(hive,"HKCR")==0) return HKEY_CLASSES_ROOT;
	if(stricmp(hive,"HKCU")==0) return HKEY_CURRENT_USER;
	if(stricmp(hive,"HKLM")==0) return HKEY_LOCAL_MACHINE;
	if(stricmp(hive,"HKUS")==0) return HKEY_USERS;

return (HKEY)0;
}
int ProcessAction()
{
	CurrentHive=SetHkey(CHive);									/* Convert to HKEY			*/
	if(strnicmp(COper,"AD",2)==0) return CreateNewKey();		/* Do and ADD				*/
	if(strnicmp(COper,"DELETEV",7)==0) return DeleteValue();	/* Delete of value name		*/
	if(strnicmp(COper,"DELETEK",7)==0) return DeleteKey();      /* Delete a Key name        */
	if(strnicmp(COper,"Ap",2)==0) return AppendValue();			/* Append to end of value   */
	if(strnicmp(COper,"RemoveS",7)==0) return RemoveString();	/* Remove a value from string*/
	if(strnicmp(COper,"Pr",2)==0) return PrependValue();		/* Prepend  of value		*/
	if(strnicmp(COper,"GetV",4)==0) return GetValue();			/* Set Value into EDM Object*/
	lmsg(MSG_ERR,"#Operation [%s] not supported\n",COper);
return 0;
}
int CreateNewKey()
{
HKEY ret;
DWORD retdisp,type;
long rc;
char cls[]="CLASS";

	if((rc=RegCreateKeyEx(CurrentHive,CKey,0,cls,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&ret,&retdisp))!=ERROR_SUCCESS) return 0;
	type=SetValueType(CDataType);						// Set type and format data if needed
	if((rc=RegSetValueEx(ret,CValue,0,type,(LPBYTE)&CData,LengthOf(CDataLength)))!=ERROR_SUCCESS) return 0;
	RegCloseKey(ret);
return 1;
}
DWORD LengthOf(DWORD w)
{
	return w;
}
int DeleteValue()
{
HKEY ret;
long rc;

	if((rc=RegOpenKeyEx(CurrentHive,CKey,0,KEY_ALL_ACCESS,&ret))!=ERROR_SUCCESS) return 0;
	if((rc=RegDeleteValue(ret,CValue))!=ERROR_SUCCESS) return 0;
	RegCloseKey(ret);
return 1;
}
int DeleteKey()
{
HKEY ret;
long rc;

	if((rc=RegDeleteKey(CurrentHive,CKey))!=ERROR_SUCCESS) 
		{
		RegCloseKey(ret); 
		return 0;
		}
	RegCloseKey(ret);
return 1;
}
int AppendValue()
{
HKEY ret;
DWORD index=0;
char vname[REGVALUENAME];
DWORD vsize=REGVALUENAME;
DWORD type=0;
DWORD dsize=DATABUFSIZE;
long rc;

	memset(vname,0,sizeof(vname));
	if((rc=RegOpenKeyEx(CurrentHive,CKey,0,KEY_ALL_ACCESS,&ret))!=ERROR_SUCCESS) return 0;
	while(1)
		{
		dsize=DATABUFSIZE;
		vsize=REGVALUENAME;
		memset(DataBuffer,0,sizeof(DataBuffer));
		rc=RegEnumValue(ret,index,vname,&vsize,0,&type,DataBuffer,&dsize);
		if(rc!=ERROR_SUCCESS) 
			{
			lmsg(MSG_ERR,"Failure in EnumValue [%d] ValueName[%s] Index[%d]\n",rc,CValue,index);
			return 0;
			}
		if(strnicmp(vname,CValue,strlen(vname))==0) break;
		index++;
		}
	if(strnicmp(vname,CValue,strlen(vname))!=0)
			{
			lmsg(MSG_ERR,"Failure to find Value Name[%s]\n",CValue);
			return 0;
			}
	strcat(DataBuffer,CData);
	if((rc=RegSetValueEx(ret,CValue,0,type,DataBuffer,strlen(DataBuffer)))!=ERROR_SUCCESS)
			{
			lmsg(MSG_ERR,"Failure in SetValue [%d]\n",rc);
			return 0;
			}
	RegCloseKey(ret);
return 1;
}
int PrependValue()
{
HKEY ret;
DWORD index=0;
char vname[REGVALUENAME];
DWORD vsize=REGVALUENAME;
DWORD type=0;
DWORD dsize=DATABUFSIZE;
long rc;
int len;

	memset(vname,0,sizeof(vname));
	memset(DataBuffer,0,sizeof(DataBuffer));
	strcpy(DataBuffer,CData);						/* Load in the new data */
	len=strlen(DataBuffer);							/* Get the length       */
	dsize=dsize-len;								/* Reset the length     */
	if((rc=RegOpenKeyEx(CurrentHive,CKey,0,KEY_ALL_ACCESS,&ret))!=ERROR_SUCCESS) return 0;
	while(1)
		{
		dsize=DATABUFSIZE;
		vsize=REGVALUENAME;
		rc=RegEnumValue(ret,index,vname,&vsize,0,&type,&DataBuffer[len],&dsize);
		if(rc!=ERROR_SUCCESS) return 0;
		if(strnicmp(vname,CValue,strlen(vname))==0) break;
		index++;
		}
	if(strnicmp(vname,CValue,strlen(vname))!=0) return 0;
	if((rc=RegSetValueEx(ret,CValue,0,type,DataBuffer,strlen(DataBuffer)))!=ERROR_SUCCESS) return 0;
	RegCloseKey(ret);
return 1;
}
int RemoveString()
{
HKEY ret;
DWORD index=0;
char vname[REGVALUENAME];
DWORD vsize=REGVALUENAME;
DWORD type=0;
DWORD dsize=DATABUFSIZE;
long rc;
int pos=0,len=0;
char *pdest;

	memset(vname,0,sizeof(vname));
	memset(DataBuffer,0,sizeof(DataBuffer));
	if((rc=RegOpenKeyEx(CurrentHive,CKey,0,KEY_ALL_ACCESS,&ret))!=ERROR_SUCCESS) return 0;
	while(1)
		{
    	memset(DataBuffer,0,sizeof(DataBuffer));
		dsize=DATABUFSIZE;
		vsize=REGVALUENAME;
		rc=RegEnumValue(ret,index,vname,&vsize,0,&type,&DataBuffer[len],&dsize);
		if(rc!=ERROR_SUCCESS) return 0;
		if(strnicmp(vname,CValue,strlen(vname))==0) break;
		index++;
		}
	if(strnicmp(vname,CValue,strlen(vname))!=0) return 0;
	pdest=strstr(DataBuffer,CData);							/* Find the value in buffer			*/
	if(!pdest) {RegCloseKey(ret); return 1; }				/* Note found all ok				*/
	pos=pdest-DataBuffer;									/* Position of data within string   */
	len=strlen(CData);										/* Length of data to remove         */
	strcpy(&DataBuffer[pos],&DataBuffer[pos+len]);			/* Now remove the data              */			
	if((rc=RegSetValueEx(ret,CValue,0,type,DataBuffer,strlen(DataBuffer)))!=ERROR_SUCCESS) return 0;
	RegCloseKey(ret);
return 1;
}

int FormatData(char *input)
{
	if(input[0]=='\"') strcpy(&input[0],&input[1]);  /* Remove double quote */
	if(input[strlen(input)-1] == '\"') input[strlen(input)-1]=0;
return 1;
}
DWORD SetValueType(char *val)
{
DWORD work=0;


	CDataLength=strlen(CData);														// Set default length
	if(stricmp(val,"REG_EXPAND_SZ")==0) return REG_EXPAND_SZ;
	if(stricmp(val,"REG_SZ")==0) return REG_SZ;
	if(stricmp(val,"REG_MULTI_SZ")==0) {formatstring(CData); return REG_MULTI_SZ;}
	if(stricmp(val,"REG_NONE")==0) return REG_NONE;
	if(stricmp(val,"REG_RESOURCE_LIST")==0) return REG_RESOURCE_LIST;
	if( strnicmp(val,"REG_DWORD",9)==0)
		{
		CDataLength=4;		
		sscanf(CData,"%lu",&work);
		memcpy(CData,(void *)&work,4);
		return REG_DWORD;
		}
	if(stricmp(val,"REG_BINARY")==0) {CDataLength=formatbinary(CData); return REG_BINARY; }
return REG_NONE;
}
int GetValue()
{
HKEY ret;
DWORD index=0;
char vname[REGVALUENAME];
DWORD vsize=REGVALUENAME;
DWORD type=0;
DWORD dsize=DATABUFSIZE;
long rc;

	memset(vname,0,sizeof(vname));
	if((rc=RegOpenKeyEx(CurrentHive,CKey,0,KEY_ALL_ACCESS,&ret))!=ERROR_SUCCESS) return 0;
	while(1)
		{
		dsize=DATABUFSIZE;
		vsize=REGVALUENAME;
		memset(DataBuffer,0,sizeof(DataBuffer));
		rc=RegEnumValue(ret,index,vname,&vsize,0,&type,DataBuffer,&dsize);
		if(rc!=ERROR_SUCCESS) 
			{
			lmsg(MSG_ERR,"Failure in EnumValue [%d] ValueName[%s] Index[%d]\n",rc,CValue,index);
			return 0;
			}
		if(strnicmp(vname,CValue,strlen(vname))==0) break;
		index++;
		}
	if(strnicmp(vname,CValue,strlen(vname))!=0)
			{
			lmsg(MSG_ERR,"Failure to find Value Name[%s]\n",CValue);
			return 0;
			}
	RegCloseKey(ret);
return dsize;
}

int formatstring(char *in)
{
int x=0;
	while(in[x]) {
		if(in[x]=='`') in[x]='\0';		// set as a string 
		x++;
	}
return x;
}
int formatbinary(char *in)
{
int x=0,y=0;
char buffer[REGDATA];
char hold[3]="  ";
	
	memset(buffer,0,sizeof(buffer));
	for (x=0;x<CDataLength;x=x+2)
		{
		memcpy(hold,&CData[x],2);
		sscanf(hold,"%x",&buffer[y++]);
		}
	memcpy(CData,buffer,y);

return y;
}
