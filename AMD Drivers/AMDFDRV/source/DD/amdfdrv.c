#include "ntddk.h"
#include "ksecdd.h"
#include "stdarg.h"
#include "stdio.h"
#include "ioctlcmdf.h"
#include "ntsec.h"
#include "amdfdrv.h"

//----------------------------------------------------------------------
//                           DEFINES
//----------------------------------------------------------------------
// print macro that only turns on when debugging is on
#if DBG
#define DbgPrint(arg) DbgPrint arg
xxxx
#else
#define DbgPrint(arg) 
#endif
#define K2	2048
#define K1	1024
#define OPEN_EXISTING	3
#define MAX_PATH	512
//
// Macro for easy hook/unhook. On X86 implementations of Zw* functions, the DWORD
// following the first byte is the system call number, so we reach into the Zw function
// passed as a parameter, and pull the number out. This makes system call hooking
// dependent ONLY on the Zw* function implementation not changing.
//

#define SYSCALL(_function)  ServiceTable->ServiceTable[ *(PULONG)((PUCHAR)_function+1)]

//
// The name of the System process, in which context we're called in our DriverEntry
//
#define SYSNAME         "System"

//
// A unicode string constant for the "default" value
//
#define DEFAULTNAMELEN  (9*sizeof(WCHAR))
WCHAR                   DefaultValueString[] = L"(Default)";
UNICODE_STRING          DefaultValue = {
    DEFAULTNAMELEN,
    DEFAULTNAMELEN,
    DefaultValueString
};


BOOLEAN AMDGetUGetLogonId(HANDLE TokenHandle, char *UserName);
extern BOOLEAN             FilterOn;
//
// A filter to use if we're monitoring boot activity
//
FILTER  BootFilter = {
    "*", "", "*", "",
    TRUE, TRUE, TRUE, TRUE,FALSE 
};

//----------------------------------------------------------------------
//                         FORWARD DEFINES
//---------------------------------------------------------------------- 
NTSTATUS AMDDispatch( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
VOID     AMDUnload( IN PDRIVER_OBJECT DriverObject );

//----------------------------------------------------------------------
//                         GLOBALS
//---------------------------------------------------------------------- 
// our user-inteface device object
PDEVICE_OBJECT          GUIDevice;

//
// Is a GUI talking to us?
//
BOOLEAN                 GUIActive = FALSE;

//
// Are we logging a boot sequence?
//
BOOLEAN                 BootLogging = FALSE;
KEVENT                  LoggingEvent;
PSTORE_BUF              BootSavedStoreList = NULL;
PSTORE_BUF              BootSavedStoreTail;

//
// Is registry hooked?
//
BOOLEAN                 RegHooked = FALSE;

//
// Global error string
//
CHAR                    errstring[256];

//
// Global filter (sent to us by the GUI)
//
FILTER                  FilterDef;
int						exlen[MAXFILTERS];			// length of each process exclude filter

//
// Lock to protect filter arrays
//
KMUTEX                  FilterMutex;

//
// Array of process and path filters 
//
ULONG                   NumProcessFilters = 0;
PCHAR                   ProcessFilters[MAXFILTERS];
ULONG                   NumProcessExcludeFilters = 0;
PCHAR                   ProcessExcludeFilters[MAXFILTERS];
ULONG                   NumPathIncludeFilters = 0;
PCHAR                   PathIncludeFilters[MAXFILTERS];
ULONG                   NumPathExcludeFilters = 0;
PCHAR                   PathExcludeFilters[MAXFILTERS];

//
// Pointer to system global service table
//
PSRVTABLE               ServiceTable;

//
// This is the offset into a KPEB of the current process name. This is determined
// dynamically by scanning the process block belonging to the GUI for its name.
//
ULONG                   ProcessNameOffset;

//
// We save off pointers to the actual Registry functions in these variables
//
NTSTATUS (*RealWriteFile)( IN HANDLE, IN HANDLE, IN PIO_APC_ROUTINE,IN PVOID,
						  OUT PIO_STATUS_BLOCK,IN PVOID, IN ULONG,IN PLARGE_INTEGER, 
						  IN PULONG);
NTSTATUS (*RealOpenFile)(OUT PHANDLE FileHandle,IN ACCESS_MASK DesiredAccess,IN POBJECT_ATTRIBUTES ObjectAttributes,
						 OUT PIO_STATUS_BLOCK IoStatusBlock,IN ULONG ShareAccess,IN ULONG OpenOptions);

NTSTATUS (*RealCreateFile)(OUT PHANDLE FileHandle,IN ACCESS_MASK DesiredAccess,IN POBJECT_ATTRIBUTES ObjectAttributes,
						 OUT PIO_STATUS_BLOCK IoStatusBlock,IN PLARGE_INTEGER AllocationSize OPTIONAL,IN ULONG FileAttributes,
						 IN ULONG ShareAccess,IN ULONG CreateDisposition,IN ULONG CreateOptions,IN PVOID EaBuffer OPTIONAL,
						 IN ULONG EaLength);
NTSTATUS (*RealSetFile) (IN HANDLE  FileHandle,OUT PIO_STATUS_BLOCK  IoStatusBlock,IN PVOID  FileInformation,IN ULONG  Length,IN FILE_INFORMATION_CLASS  FileInformationClass);


NTSTATUS (*RealClose) (IN HANDLE  Handle);

int ConvertToUpper( PCHAR Dest, PCHAR Source, ULONG Len );
//
// Are we logging 
//
#define LOGLEN 256
NTSTATUS AMDFDRV_OpenLog(void);
NTSTATUS AMDFDRV_AppLog();
void writetofile(PWSTR pfl,char *addr,int nsize);

HANDLE                  LogFile = INVALID_HANDLE_VALUE;
UNICODE_STRING			LogFileName;
CHAR                    svclogname[LOGLEN];
PCHAR ErrorString( NTSTATUS retval );
char lname[]="\\DosDevices\\C:\\amdlog.log";
WCHAR wname[]=L"\\DosDevices\\C:\\amdlog.log";

//
// Data structure for storing messages we generate
//
PSTORE_BUF              Store           = NULL;
ULONG                   Sequence        = 0;
KMUTEX                  StoreMutex;

//
// This is a hash table for keeping names around for quick lookup.
//
PHASH_ENTRY             HashTable[NUMHASH];

//
// Mutex for hash table accesses
//
KMUTEX                  HashMutex;


//
// Maximum amount of data we will grab for buffered unread data
//
ULONG                   NumStore        = 0;
ULONG                   MaxStore        = MAXMEM/MAX_STORE;

//
// Free hash list. Note: we don't use lookaside lists since
// we want to be able to run on NT 3.51 - lookasides were
// introduced in NT 4.0
//
PHASH_ENTRY             FreeHashList = NULL;


//======================================================================
//                   B U F F E R  R O U T I N E S 
//======================================================================

//----------------------------------------------------------------------
//
// AMDFreeStore
//
// Frees all the data output buffers that we have currently allocated.
//
//----------------------------------------------------------------------
VOID AMDFreeStore()
{
    PSTORE_BUF      next;
    
    while( Store ) {
        next = Store->Next;
        ExFreePool( Store );
        Store = next;
    }
}       


//----------------------------------------------------------------------
//
// AMDNewStore
//
// Called when the current buffer has filled up. This moves us to the
// pre-allocated buffer and then allocates another buffer.
//
//----------------------------------------------------------------------
void AMDNewStore( void )
{
    PSTORE_BUF prev = Store, newstore;
    WORK_QUEUE_ITEM  workItem;

    //
    // If we have maxed out or haven't accessed the current store
    // just return
    //
    if( MaxStore == NumStore ) {
        Store->Len = 0;
        return; 
    }

    //
    // See if we can re-use a store
    //
    if( !Store->Len ) {

        return;
    }

    //
    // Move to the next buffer and allocate another one
    //
    newstore = ExAllocatePool( PagedPool, sizeof(*Store) );
    if( newstore ) { 

        Store   = newstore;
        Store->Len  = 0;
        Store->Next = prev;
        NumStore++;

    } else {

        Store->Len = 0;

    }
}


//----------------------------------------------------------------------
//
// AMDOldestStore
//
// Goes through the linked list of storage buffers and returns the 
// oldest one.
//
//----------------------------------------------------------------------
PSTORE_BUF AMDOldestStore( void )
{
    PSTORE_BUF  ptr = Store, prev = NULL;

    while ( ptr->Next ) {

        ptr = (prev = ptr)->Next;
    }
    if ( prev ) {

        prev->Next = NULL;    
    }
    NumStore--;
    return ptr;
}


//----------------------------------------------------------------------
//
// AMDResetStore
//
// When a GUI is no longer communicating with us, but we can't unload,
// we reset the storage buffers.
//
//----------------------------------------------------------------------
VOID AMDResetStore()
{
    PSTORE_BUF  current, next;

    MUTEX_WAIT( StoreMutex );

    //
    // Traverse the list of output buffers
    //
    current = Store->Next;
    while( current ) {

        //
        // Free the buffer
        //
        next = current->Next;
        ExFreePool( current );
        current = next;
    }

    // 
    // Move the output pointer in the buffer that's being kept
    // the start of the buffer.
    // 
    Store->Len = 0;
    Store->Next = NULL;

    MUTEX_RELEASE( StoreMutex );
}



//----------------------------------------------------------------------
//
// UpdateStore
//
// Add a new string to Store, if it fits. 
//
//----------------------------------------------------------------------
void UpdateStore( const char * format, ... )
{       
    PENTRY          Entry;
    ULONG           len;
    va_list         arg_ptr;
    static CHAR     text[MAXPATHLEN*2];
	IO_STATUS_BLOCK ioStatus;


    //
    // only do this if a GUI is active
    //
//    if( !GUIActive ) return;

    //
    // Lock the buffer pool
    //
    MUTEX_WAIT( StoreMutex );

    //
    // Get a sequence numnber
    //
    InterlockedIncrement( &Sequence );

    //
    // Sprint the string to get the length
    //
    va_start( arg_ptr, format );
    len = vsprintf( text, format, arg_ptr );
    va_end( arg_ptr ); 

	//writetofile(wname,text,len);

	len += 4; len &=  0xFFFFFFFC; // +1 to include null terminator and +3 to allign on longword


    //
    // See if its time to switch to extra buffer
    //
    if ( Store->Len + len + sizeof(ENTRY) + 1 >= MAX_STORE ) {

        AMDNewStore();
    }

    //
    // Store the sequence number so that 
    // a call's result can be paired with its
    // initial data collected when it was made.
    //
    Entry = (void *)(Store->Data+Store->Len);
    Entry->seq = Sequence;
    memcpy( Entry->text, text, len );
    Store->Len += sizeof(Entry->seq)+len;

    //
    // Release the buffer pool
    //
    MUTEX_RELEASE( StoreMutex );
}


//----------------------------------------------------------------------
// 
// strncatZ
//
// Appends a string to another and attaches a null. NT 3.51 ntoskrnl
// does not export this function so we have to make our own, and give
// it a name that won't conflict with the strncat that NT 4.0 exports.
//
//----------------------------------------------------------------------
PCHAR strncatZ( PCHAR dest, PCHAR source, int length )
{       
    int     origlen = strlen(dest);

    strncpy( dest+origlen, source, length );
    dest[ origlen+length ] = 0;
    return(dest);
}


//======================================================================
//  R E G I S T R Y  P A R A M E T E R  S U P P O R T  R O U T I N E S
//======================================================================

//----------------------------------------------------------------------
//
// ConverToUpper
//
// Obvious.
//
//----------------------------------------------------------------------
int ConvertToUpper( PCHAR Dest, PCHAR Source, ULONG Len )
{
    int   i;
    
    for( i = 0; i < (int)Len; i++ ) {
		if (Source[i]<' ' || Source[i]>'~') return i;
        if( Source[i] >= 'a' && Source[i] <= 'z' ) { Dest[i] = Source[i] - 'a' + 'A'; Dest[i+1]='\0';}
        else {Dest[i] = Source[i]; Dest[i+1]='\0';}
		}
	return i;
}



//----------------------------------------------------------------------
//
// GetPointer
//
// Translates a handle to an object pointer.
//
//----------------------------------------------------------------------
POBJECT GetPointer( HANDLE handle )
{
    POBJECT         pKey;

    //
    // Ignore null handles
    //
    if( !handle ) return NULL;

    //
    // Get the pointer the handle refers to
    //
    if( ObReferenceObjectByHandle( handle, 0, NULL, KernelMode, &pKey, NULL ) !=
        STATUS_SUCCESS ) {

        pKey = NULL;
    } 
    return pKey;
}


//----------------------------------------------------------------------
//
// ReleasePointer
//
// Dereferences the object.
//
//----------------------------------------------------------------------
VOID ReleasePointer( POBJECT object )
{
    if( object ) ObDereferenceObject( object );
}


//----------------------------------------------------------------------
//
// AMDFreeFilters
//
// Fress storage we allocated for filter strings.
//
//----------------------------------------------------------------------
VOID AMDFreeFilters()
{
    ULONG   i;

    for( i = 0; i < NumProcessFilters; i++ ) {

        ExFreePool( ProcessFilters[i] );
    }
    for( i = 0; i < NumProcessExcludeFilters; i++ ) {

        ExFreePool( ProcessExcludeFilters[i] );
    }
    for( i = 0; i < NumPathIncludeFilters; i++ ) {

        ExFreePool( PathIncludeFilters[i] );
    }
    for( i = 0; i < NumPathExcludeFilters; i++ ) {

        ExFreePool( PathExcludeFilters[i] );
    }
    NumProcessFilters = 0;
    NumProcessExcludeFilters = 0;
    NumPathIncludeFilters = 0;
    NumPathExcludeFilters = 0;
}

//----------------------------------------------------------------------
//
// MakeFilterArray
//
// Takes a filter string and splits into components (a component
// is seperated with a ';')
//
//----------------------------------------------------------------------
VOID MakeFilterArray( PCHAR FilterString,PCHAR FilterArray[],PULONG NumFilters )
{
    PCHAR filterStart;
    ULONG filterLength;

    //
    // Scan through the process filters
    //
    filterStart = FilterString;
    while( *filterStart ) {

        filterLength = 0;
        while( filterStart[filterLength] &&
               filterStart[filterLength] != ';' ) {

            filterLength++;
        }

        //
        // Ignore zero-length components
        //
        if( filterLength ) {

            FilterArray[ *NumFilters ] = 
                ExAllocatePool( PagedPool, filterLength + 1 );
            strncpy( FilterArray[ *NumFilters ],
                     filterStart, filterLength );
            FilterArray[ *NumFilters ][filterLength] = 0;
            (*NumFilters)++;
		if( *NumFilters == MAXFILTERS) break;	// End of the line
        }
    
        //
        // Are we done?
        //
        if( !filterStart[filterLength] ) break;

        //
        // Move to the next component (skip over ';')
        //
        filterStart += filterLength + 1;
    }
}

//----------------------------------------------------------------------
//
// AMDUpdateFilters
//
// Takes a new filter specification and updates the filter
// arrays with them.
//
//----------------------------------------------------------------------
VOID AMDUpdateFilters()
{
ULONG i;
    //
    // Free old filters (if any)
    //
    MUTEX_WAIT( FilterMutex );

    AMDFreeFilters();

    //
    // Create new filter arrays
    //
    MakeFilterArray( FilterDef.processfilter,
                     ProcessFilters, &NumProcessFilters );
    MakeFilterArray( FilterDef.processexclude,
                     ProcessExcludeFilters, &NumProcessExcludeFilters );
    for( i = 0; i < NumProcessExcludeFilters; i++ ) {
        exlen[i]=strlen(ProcessExcludeFilters[i]);
		}
    MakeFilterArray( FilterDef.pathfilter,
                     PathIncludeFilters, &NumPathIncludeFilters );
    MakeFilterArray( FilterDef.excludefilter,
                     PathExcludeFilters, &NumPathExcludeFilters );    

    MUTEX_RELEASE( FilterMutex );
}


//----------------------------------------------------------------------
//
// ApplyNameFilter
//
// If the name matches the exclusion mask, we do not log it. Else if
// it doesn't match the inclusion mask we do not log it. 
//
//----------------------------------------------------------------------
LONG ApplyNameFilter( PCHAR fullname )
{
    ULONG    i,x,y;

    //   
    // If it matches the exclusion string, do not log it
    //
    MUTEX_WAIT( FilterMutex );

    for( i = 0; i < NumPathExcludeFilters; i++ ) {

        if( (strstr( fullname , PathExcludeFilters[i] )) ) {
            MUTEX_RELEASE( FilterMutex );
            return -2;
        }
    }
 
    //
    // If it matches an include filter then log it
    //
    for( i = 0; i < NumPathIncludeFilters; i++ ) {
		for(x=0, y=0;PathIncludeFilters[i][x] > ' '; x++,y++)
			{
			if(PathIncludeFilters[i][x]=='*') {												// Section Wild card
				while(PathIncludeFilters[i][x]>' ' && PathIncludeFilters[i][x] != '\\') x++;
				while(fullname[y]> ' ' && fullname[y] != '\\') y++;
				}
			if(PathIncludeFilters[i][x] != fullname[y]) break;								// Not this guy
			if(PathIncludeFilters[i][x]=='\0') {MUTEX_RELEASE( FilterMutex ); return i;}	// Match for this mask
			}
		if(PathIncludeFilters[i][x]=='\0') {MUTEX_RELEASE( FilterMutex ); return i;}	// Match for this mask
		}

    //
    // It didn't match any include filters so don't log
    //
    MUTEX_RELEASE( FilterMutex );
    return -1;
}


//----------------------------------------------------------------------
//
// GetProcessNameOffset
//
// In an effort to remain version-independent, rather than using a
// hard-coded into the KPEB (Kernel Process Environment Block), we
// scan the KPEB looking for the name, which should match that
// of the GUI process
//
//----------------------------------------------------------------------
ULONG GetProcessNameOffset()
{
    PEPROCESS       curproc;
    int             i;

    curproc = PsGetCurrentProcess();

    //
    // Scan for 12KB, hopping the KPEB never grows that big!
    //
    for( i = 0; i < 3*PAGE_SIZE; i++ ) {
     
        if( !strncmp( SYSNAME, (PCHAR) curproc + i, strlen(SYSNAME) )) {

            return i;
        }
    }

    //
    // Name not found - oh, well
    //
    return 0;
}



//----------------------------------------------------------------------
//
// GetProcess
//
// Uses undocumented data structure offsets to obtain the name of the
// currently executing process.
//
//----------------------------------------------------------------------
PCHAR GetProcess( PCHAR Name )
{
    PEPROCESS       curproc;
    char            *nameptr;

    //
    // We only try and get the name if we located the name offset
    //
    if( ProcessNameOffset ) {
        curproc = PsGetCurrentProcess();
        nameptr   = (PCHAR) curproc + ProcessNameOffset;
        strncpy( Name, nameptr, 16 );
		} 
	else strcpy( Name, "Unknown");

    return NULL;
}

//======================================================================
//                    H O O K  R O U T I N E S
//======================================================================
NTSTATUS HookClose(IN HANDLE han)
{
NTSTATUS				qstat,rtlstat;
IO_STATUS_BLOCK			sb;
PFILE_NAME_INFORMATION	fn;
ULONG					ln=K2;
ANSI_STRING				afilen;
UNICODE_STRING			ufilen;
char					fname[K1];
char					fbuff[K2];
HANDLE					tokenHandle=NULL;
char					userName[SIDCHARSIZE];
char					uname[MAXPROCNAMELEN];
char					name[MAXPROCNAMELEN];
int						texe=1;
ULONG					i=0;

	GetProcess(name);
	ConvertToUpper(uname,name,strlen(name));

	(char *)fn=fbuff;
	qstat=ZwQueryInformationFile(han,&sb,fn,ln,FileNameInformation);

	if(NT_SUCCESS(qstat)) {
		RtlInitUnicodeString(&ufilen,(PCWSTR)fn->FileName);
		rtlstat=RtlUnicodeStringToAnsiString( &afilen,&ufilen,TRUE ); 
		ConvertToUpper(fname,afilen.Buffer,afilen.Length);
		}

	MUTEX_WAIT( FilterMutex );
      for( i = 0; i < NumProcessExcludeFilters; i++ ) {
        if( (strstr( uname , ProcessExcludeFilters[i] )) ) {texe=0; break;}
		if(ProcessExcludeFilters[i][0]=='*') {texe=0; break;}
		}
    MUTEX_RELEASE( FilterMutex );

	if(!texe) return RealClose(han);


	RtlZeroMemory( userName, SIDCHARSIZE );
    tokenHandle = AMDGetUOpenEffectiveToken();

	if(tokenHandle) {	
			AMDGetUGetLogonId(tokenHandle,&userName[0]);
			}
	else strcpy(userName,"Unknown\\Unknown");

	UpdateStore( ":%s:TrackFile;%s;%s;CloseFile",userName, uname,fname);

	return RealClose(han);
}
//----------------------------------------------------------------------
//
// HookSetInformationFile
// UnUsed at this time. Only need this is we want the driver itself to 
// track the delete of a file. 
//----------------------------------------------------------------------

NTSTATUS HookSetFile (HANDLE han,PIO_STATUS_BLOCK IoS,PVOID FI,ULONG Length,FILE_INFORMATION_CLASS FIClass)
{
NTSTATUS				qstat,rtlstat;
IO_STATUS_BLOCK			sb;
PFILE_NAME_INFORMATION	fn;
ULONG					ln=K2;
ANSI_STRING				afilen;
UNICODE_STRING			ufilen;
char					fname[K1];
char					fbuff[K2];
HANDLE					tokenHandle=NULL;
char					userName[SIDCHARSIZE];
char					uname[MAXPROCNAMELEN];
char					name[MAXPROCNAMELEN];
int						texe=1;
ULONG					i=0;
LARGE_INTEGER			now;


	return RealSetFile(han,IoS,FI,Length,FIClass);
	if(FIClass!=FileDispositionInformation) return RealSetFile(han,IoS,FI,Length,FIClass);
	GetProcess(name);
	ConvertToUpper(uname,name,strlen(name));

	MUTEX_WAIT( FilterMutex );
      for( i = 0; i < NumProcessExcludeFilters; i++ ) {
        if( (strstr( uname , ProcessExcludeFilters[i] )) ) {texe=0; break;}
		if(ProcessExcludeFilters[i][0]=='*') {texe=0; break;}
		}
    MUTEX_RELEASE( FilterMutex );

	if(!texe) return RealSetFile(han,IoS,FI,Length,FIClass);

	(char *)fn=fbuff;
	qstat=ZwQueryInformationFile(han,&sb,fn,ln,FileNameInformation);

	if(NT_SUCCESS(qstat)) {
		RtlInitUnicodeString(&ufilen,(PCWSTR)fn->FileName);
		rtlstat=RtlUnicodeStringToAnsiString( &afilen,&ufilen,TRUE ); 
		ConvertToUpper(fname,afilen.Buffer,afilen.Length);
		}

	RtlZeroMemory( userName, SIDCHARSIZE );
    tokenHandle = AMDGetUOpenEffectiveToken();

	if(tokenHandle) {	
			AMDGetUGetLogonId(tokenHandle,&userName[0]);
			}
	else strcpy(userName,"Unknown\\Unknown");

	KeQuerySystemTime(&now);
	UpdateStore( ":%s:TrackFileDelete;%s;%s;TIME(%ul:%ul)",userName,uname,fname,now.HighPart,now.LowPart);

	return RealSetFile(han,IoS,FI,Length,FIClass);
}
//----------------------------------------------------------------------
//
// HookWriteFile
//
//----------------------------------------------------------------------
NTSTATUS HookWriteFile(IN HANDLE fh, IN HANDLE evt, IN PIO_APC_ROUTINE apc,IN PVOID context,OUT PIO_STATUS_BLOCK st,IN PVOID buf , IN ULONG len,IN PLARGE_INTEGER off,IN PULONG key)
{
    NTSTATUS				ntstatus;
	NTSTATUS				qstat,rtlstat;
	IO_STATUS_BLOCK			sb;
	PFILE_NAME_INFORMATION	fn;
	ULONG					ln=K2;
	ANSI_STRING				afilen;
	UNICODE_STRING			ufilen;
	char					name[MAXPROCNAMELEN];
	char					uname[MAXPROCNAMELEN];
	char					fbuff[K2];
	char					fname[K1];
    HANDLE					tokenHandle=NULL;
    char					userName[SIDCHARSIZE];
	char					*p;
	int						texe=0;
	ULONG					i;

	//
	// The theory here is that if all is well then just keep on moving
	// We will generate an error due to the fact that we reset the write flags on the
	// open. Therefore any error we want to look at else forget it. This way its like
	// We are not even loaded. No impact on the OS's performance;
	//
	ntstatus=RealWriteFile(fh,evt,apc,context,st,buf,len,off,key);
//	if (FilterDef.logwrites || NT_SUCCESS(ntstatus) || FilterDef.logsuccess)  {
	if (NT_SUCCESS(ntstatus)) return ntstatus;


	RtlZeroMemory( name, MAXPROCNAMELEN ); 
	RtlZeroMemory( uname, MAXPROCNAMELEN ); 
	RtlZeroMemory( fname, K1 ); 
	RtlZeroMemory( fbuff, K2 ); 
	(char *)fn=fbuff;


	GetProcess(name);
	ConvertToUpper(uname,name,strlen(name));

	qstat=ZwQueryInformationFile(fh,&sb,fn,ln,FileNameInformation);

	if(NT_SUCCESS(qstat)) {
		RtlInitUnicodeString(&ufilen,(PCWSTR)fn->FileName);
		rtlstat=RtlUnicodeStringToAnsiString( &afilen,&ufilen,TRUE ); 
		ConvertToUpper(fname,afilen.Buffer,afilen.Length);
		}
	else return ntstatus;

	if( (strlen(fname))==0) return ntstatus;

	if(p=strstr(fname,".EXE")) texe=1;
	if(p=strstr(fname,".DLL")) texe=1;
	MUTEX_WAIT( FilterMutex );
      for( i = 0; i < NumProcessExcludeFilters; i++ ) {
        if( (strstr( uname , ProcessExcludeFilters[i] )) ) {texe=0; break;}
		if(ProcessExcludeFilters[i][0]=='*') {texe=0; break;}
		}
      for( i = 0; i < NumProcessFilters; i++ ) {
        if( (strstr( fname , ProcessFilters[i] )) ) {texe=1; break;}
		}

    MUTEX_RELEASE( FilterMutex );


	if(!texe) return ntstatus;

	RtlZeroMemory( userName, SIDCHARSIZE );
    tokenHandle = AMDGetUOpenEffectiveToken();

	if(tokenHandle) {	
			AMDGetUGetLogonId(tokenHandle,&userName[0]);
			}
	else strcpy(userName,"Unknown\\Unknown");

	UpdateStore( ":%s:TrackFile;%s;%s;Invalid Action",userName, uname,fname);
    return ntstatus;
}

//----------------------------------------------------------------------
//
// HookOpenFile
//
//----------------------------------------------------------------------
NTSTATUS HookOpenFile(OUT PHANDLE FileHandle,IN ACCESS_MASK DesAcc,IN POBJECT_ATTRIBUTES ObjAttr,
						 OUT PIO_STATUS_BLOCK IoStatusBlock,IN ULONG ShareAccess,IN ULONG OpenOptions)
{
    NTSTATUS				ntstatus,rtlstat;
	char					name[MAXPROCNAMELEN];
	char					uname[MAXPROCNAMELEN];
	UNICODE_STRING			pfilen;
	ANSI_STRING				afilen;
	int						wraccess=0,exemode=0,nlen=0,exl=0;
	ULONG					i=0;
	char					fname[K1];
	FILE_BASIC_INFORMATION	fb;
	IO_STATUS_BLOCK			sb;
	LARGE_INTEGER			now;
	HANDLE					lhand;
	char					fbuff[MAX_PATH];
	PFILE_NAME_INFORMATION	fn;
    HANDLE					tokenHandle;
    char					userName[SIDCHARSIZE];


	if(FilterDef.logsuccess) {  /* We were asked by our own process to stop locking */
		return RealOpenFile(FileHandle,DesAcc,ObjAttr,IoStatusBlock,ShareAccess,OpenOptions);
		}
	//
	// Determine is any writes enabled??? 
	// Read only can't modify anything. Don't care.... 	
	//
	if ( (DesAcc & FILE_WRITE_DATA) || (DesAcc & FILE_WRITE_ATTRIBUTES) || (DesAcc & GENERIC_WRITE) || (DesAcc & DELETE ) ) wraccess=1;
	if(!wraccess) return RealOpenFile(FileHandle,DesAcc,ObjAttr,IoStatusBlock,ShareAccess,OpenOptions);
	//
	// We check to see if the process is excluded first. Ver fast just a couple of memory copies
	//
	RtlZeroMemory( name, MAXPROCNAMELEN );
	GetProcess(name);
	nlen=ConvertToUpper(uname,name,MAXPROCNAMELEN);
    for( i = 0; i < NumProcessExcludeFilters; i++ ) {
        if( !(memcmp(ProcessExcludeFilters[i], uname,exlen[i])) ) {exl=1; break;}
		}
	if(exl) return RealOpenFile(FileHandle,DesAcc,ObjAttr,IoStatusBlock,ShareAccess,OpenOptions);
	//
	// Now we need the targets name
	//
	rtlstat=RtlUnicodeStringToAnsiString( &afilen,ObjAttr->ObjectName,TRUE ); 
	ConvertToUpper(fname,afilen.Buffer,afilen.Length);
	if((strstr(fname,".EXE")) ) exemode=1;
	if((strstr(fname,".DLL")) ) exemode=1;
	//
	// Protecting more than just exe's?
	//
    for( i = 0; i < NumProcessFilters; i++ ) {
        if( (strstr( fname , ProcessFilters[i] )) ) {wraccess=1; exemode=1; break;}
		}
	if(exemode && FilterDef.logwrites) {							// If we are blocking exe's then remove write
		DesAcc = GENERIC_READ | GENERIC_EXECUTE ;
		ShareAccess = FILE_SHARE_READ;
		}
	//
	// Now call Open file and get full path name
	//
	ntstatus=RealOpenFile(FileHandle,DesAcc,ObjAttr,IoStatusBlock,ShareAccess,OpenOptions);
	RtlZeroMemory( fname, MAX_PATH );
	lhand=*(FileHandle);
	(char *)fn=fbuff;
	RtlZeroMemory(fbuff,MAX_PATH);
	rtlstat=ZwQueryInformationFile(lhand,&sb,fn,MAX_PATH,FileNameInformation);
	if(NT_SUCCESS(rtlstat)) {
		RtlInitUnicodeString(&pfilen,(PCWSTR)fn->FileName);
		RtlUnicodeStringToAnsiString(&afilen,&pfilen,TRUE);
		nlen=ConvertToUpper(fname,afilen.Buffer,afilen.Length);
		}
	else return ntstatus;

	//
	// Get user information
	//
    tokenHandle = AMDGetUOpenEffectiveToken();
	RtlZeroMemory( userName, SIDCHARSIZE );
	if(tokenHandle) AMDGetUGetLogonId(tokenHandle,&userName[0]);


	if(exemode) {
		UpdateStore( ":%s:OpenFile;%s;%s;Write Disabled",userName, uname,fname);
		}
	KeQuerySystemTime(&now);
	UpdateStore( ":%s:TrackFile;%s;%s;TIME(%ul:%ul)",userName,uname,fname,now.HighPart,now.LowPart);

    return ntstatus;
}

//----------------------------------------------------------------------
//
// HookCreateFile
//
//----------------------------------------------------------------------
NTSTATUS HookCreateFile(OUT PHANDLE FileHandle,IN ACCESS_MASK DesAcc,IN POBJECT_ATTRIBUTES ObjAttr,
						 OUT PIO_STATUS_BLOCK IoStatusBlock,IN PLARGE_INTEGER AllocationSize OPTIONAL,IN ULONG FileAttributes,
						 IN ULONG ShareAccess,IN ULONG CreateDisposition,IN ULONG CreateOptions,IN PVOID EaBuff,IN ULONG EaLength)
{
    NTSTATUS				ntstatus,rtlstat;
	char					name[MAXPROCNAMELEN];
	char					uname[MAXPROCNAMELEN];
	ANSI_STRING				afilen;
	UNICODE_STRING			pfilen;
	char					fname[MAX_PATH];
	char					fbuff[MAX_PATH];
	PFILE_NAME_INFORMATION	fn;
	int						wraccess=0;
	int						exemode=0;
	int						nlen=0;
	int						exl=0;
	ULONG					i=0;
    HANDLE					tokenHandle;
    char					userName[SIDCHARSIZE];
	FILE_BASIC_INFORMATION	fb;
	IO_STATUS_BLOCK			sb;
	LARGE_INTEGER			now;
    PFILE_OBJECT			fileObject;
	HANDLE					lhand;
	POBJECT_HANDLE_INFORMATION info=NULL;



	if(FilterDef.logsuccess) {  /* We were asked by our own process to stop locking */
		return RealCreateFile(FileHandle,DesAcc,ObjAttr,IoStatusBlock,AllocationSize,FileAttributes,		ShareAccess,CreateDisposition,CreateOptions,EaBuff,EaLength);
		}
	//
	// Determine is any writes enabled??? 
	// Read only can't modify anything. Don't care.... 	
	//
	if ( (DesAcc & FILE_WRITE_DATA) || (DesAcc & FILE_WRITE_ATTRIBUTES) || (DesAcc & GENERIC_WRITE) || (DesAcc & DELETE ) ) wraccess=1;
	if (!wraccess) return RealCreateFile(FileHandle,DesAcc,ObjAttr,IoStatusBlock,AllocationSize,FileAttributes,ShareAccess,CreateDisposition,CreateOptions,EaBuff,EaLength);

	//
	// We check to see if the process is excluded first. Ver fast just a couple of memory copies
	//
	RtlZeroMemory( name, MAXPROCNAMELEN );
	RtlZeroMemory( uname,MAXPROCNAMELEN );
	GetProcess(name);
	nlen=ConvertToUpper(uname,name,MAXPROCNAMELEN);
    for( i = 0; i < NumProcessExcludeFilters; i++ ) {
        if( !(memcmp(ProcessExcludeFilters[i], uname,exlen[i])) ) {exl=1; break;}
		}	
	if(exl)	return RealCreateFile(FileHandle,DesAcc,ObjAttr,IoStatusBlock,AllocationSize,FileAttributes,ShareAccess,CreateDisposition,CreateOptions,EaBuff,EaLength);
	//
	// Now we need the targets name
	//
	rtlstat=RtlUnicodeStringToAnsiString( &afilen,ObjAttr->ObjectName,TRUE ); 
	ConvertToUpper(fname,afilen.Buffer,afilen.Length);
	//if((strstr(fname,"\\??\\")) ) return RealCreateFile(FileHandle,DesAcc,ObjAttr,IoStatusBlock,AllocationSize,FileAttributes,ShareAccess,CreateDisposition,CreateOptions,EaBuff,EaLength);
	if((strstr(fname,".EXE")) ) exemode=1;
	if((strstr(fname,".DLL")) ) exemode=1;
    for( i = 0; i < NumProcessFilters; i++ ) {
		if( (strstr( fname , ProcessFilters[i] )) ) {wraccess=1; exemode=1; break;}
		}
	if(exemode && FilterDef.logwrites) {							// If we are blocking exe's then remove write
		DesAcc = GENERIC_READ | GENERIC_EXECUTE ;
		CreateDisposition = FILE_OPEN;
		ShareAccess = FILE_SHARE_READ;
		CreateOptions=0;
		}
	ntstatus=RealCreateFile(FileHandle,DesAcc,ObjAttr,IoStatusBlock,AllocationSize,FileAttributes,ShareAccess,CreateDisposition,CreateOptions,EaBuff,EaLength);
	if(NT_SUCCESS(ntstatus) ) {
		RtlZeroMemory( fname, MAX_PATH );
		lhand=*(FileHandle);											// Now get path information 
		(char *)fn=fbuff;
		RtlZeroMemory(fbuff,MAX_PATH);
		rtlstat=ZwQueryInformationFile(lhand,&sb,fn,MAX_PATH,FileNameInformation);
		if(NT_SUCCESS(rtlstat)) {
			RtlInitUnicodeString(&pfilen,(PCWSTR)fn->FileName);
			RtlUnicodeStringToAnsiString(&afilen,&pfilen,TRUE);
			nlen=ConvertToUpper(fname,afilen.Buffer,afilen.Length);
			}
		else return ntstatus;
		}

	//
	// Build user information
	//
    tokenHandle = AMDGetUOpenEffectiveToken();
	RtlZeroMemory( userName, SIDCHARSIZE );
	if(tokenHandle) AMDGetUGetLogonId(tokenHandle,&userName[0]);

	if( IoStatusBlock->Information==FILE_CREATED) {
		KeQuerySystemTime(&now);
		UpdateStore( ":%s:TrackFileAdd;%s;%s;TIME(%ul:%ul)",userName,uname,fname,now.HighPart,now.LowPart);
		return ntstatus;
		}
	else {
		KeQuerySystemTime(&now);
		UpdateStore( ":%s:TrackFile;%s;%s;TIME(%ul:%ul)",userName,uname,fname,now.HighPart,now.LowPart);
		return ntstatus;
		}
	//if(exemode && FilterDef.logwrites) {
	//	UpdateStore( ":%s:CreateFile;%s;%s;Write Denied",userName,uname,fname);
	// 	return ntstatus;
	//	}

	//else {
	//	KeQuerySystemTime(&now);
	//	UpdateStore( ":%s:TrackFile;%s;%s;TIME(%ul:%ul)",userName,uname,fname,now.HighPart,now.LowPart);
	//	}


	return ntstatus;
}


//----------------------------------------------------------------------
//
// HookZWRoutines
//
// Replaces entries in the system service table with pointers to
// our own hook routines. We save off the real routine addresses.
//
//----------------------------------------------------------------------
VOID HookZWRoutines( void )
{

    if( !RegHooked ) {
//        RealWriteFile = SYSCALL( ZwWriteFile );
//        SYSCALL( ZwWriteFile ) = (PVOID) HookWriteFile;
_asm
{
CLI //dissable interrupt
MOV EAX, CR0 //move CR0 register into EAX
AND EAX, NOT 10000H //disable WP bit 
MOV CR0, EAX //write register back
}

        RealOpenFile = SYSCALL( ZwOpenFile );
        SYSCALL( ZwOpenFile ) = (PVOID) HookOpenFile;
        RealCreateFile = SYSCALL( ZwCreateFile );
        SYSCALL( ZwCreateFile ) = (PVOID) HookCreateFile;
        RealWriteFile = SYSCALL( ZwWriteFile );
        SYSCALL( ZwWriteFile ) = (PVOID) HookWriteFile;
//        RealSetFile= SYSCALL( ZwSetInformationFile);
//        SYSCALL( ZwSetInformationFile ) = (PVOID) HookSetFile;



_asm 
{
MOV EAX, CR0 //move CR0 register into EAX
OR EAX, 10000H //enable WP bit 
MOV CR0, EAX //write register back 
STI //enable interrupt
}
        RegHooked = TRUE;
    }

}


//----------------------------------------------------------------------
//
// UnHookZWRoutines
//
// Unhooks all registry routines by replacing the hook addresses in 
// the system service table with the real routine addresses that we
// saved off.
//
//----------------------------------------------------------------------
VOID UnHookZWRoutines( )
{
    if( RegHooked ) {

//        SYSCALL( ZwWriteFile ) = (PVOID) RealWriteFile;
//        SYSCALL( ZwOpenFile )    = (PVOID) RealOpenFile;
//        SYSCALL( ZwCreateFile )  = (PVOID) RealCreateFile;

//        RegHooked = FALSE;
		}
}

//======================================================================
//         D E V I C E - D R I V E R  R O U T I N E S 
//======================================================================

//----------------------------------------------------------------------
//
// AMDDeviceControl
//
//----------------------------------------------------------------------
BOOLEAN  AMDDeviceControl( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait,
                              IN PVOID InputBuffer, IN ULONG InputBufferLength, 
                              OUT PVOID OutputBuffer, IN ULONG OutputBufferLength, 
                              IN ULONG IoControlCode, OUT PIO_STATUS_BLOCK IoStatus, 
                              IN PDEVICE_OBJECT DeviceObject ) {
    BOOLEAN                 retval = FALSE;
    PSTORE_BUF              old;

    //
    // Its a message from our GUI!
    //
    IoStatus->Status      = STATUS_SUCCESS; // Assume success
    IoStatus->Information = 0;              // Assume nothing returned
    switch ( IoControlCode ) {

    case AMDDRV_version:

        //
        // Version #
        //
        if ( OutputBufferLength < sizeof(ULONG) ||
             OutputBuffer == NULL ) {

            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }
            
        *(ULONG *)OutputBuffer = AMDDRVVERSION;
        IoStatus->Information = sizeof(ULONG);
        break;

    case AMDDRV_hook:
        HookZWRoutines();
        break;

    case AMDDRV_unhook:
        UnHookZWRoutines();
        break;

    case AMDDRV_zerostats:

        //
        // Zero contents of buffer
        //
        MUTEX_WAIT( StoreMutex );
        while ( Store->Next )  {
            // release next
            old = Store->Next;
            Store->Next = old->Next;
            MUTEX_WAIT( StoreMutex );
            ExFreePool( old );
            NumStore--;
            MUTEX_RELEASE( StoreMutex );
        }
        Store->Len = 0;
        Sequence = 0;
        MUTEX_RELEASE( StoreMutex );
        break;

    case AMDDRV_getstats:

        //
        // Copy buffer into user space.
        //

        MUTEX_WAIT( StoreMutex );
        if ( MAX_STORE > OutputBufferLength )  {

            //
            // Output buffer isn't big enough
            //
            MUTEX_RELEASE( StoreMutex );
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            return FALSE;

        } else if ( Store->Len  ||  Store->Next ) {

            //
            // Switch to a new store
            //
            AMDNewStore();

            // Fetch the oldest to give to user
            old = AMDOldestStore();
            MUTEX_RELEASE( StoreMutex );

            //
            // Copy it
            //
            memcpy( OutputBuffer, old->Data, old->Len );

            //
            // Return length of copied info
            //
            IoStatus->Information = old->Len;

            //
            // Deallocate buffer
            //
            ExFreePool( old );

        } else {
            //
            // No unread data
            //
            MUTEX_RELEASE( StoreMutex );
            IoStatus->Information = 0;
        }
        break;

    case AMDDRV_setfilter:
        //        
        // GUI is updating the filter
        //
        FilterDef = *(PFILTER) InputBuffer;
        AMDUpdateFilters();
		GUIActive = TRUE;
        break;
 
    default:
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    return TRUE;
}


//----------------------------------------------------------------------
//
// AMDDispatch
//
// In this routine we handle requests to our own device. The only 
// requests we care about handling explicitely are IOCTL commands that
// we will get from the GUI. We also expect to get Create and Close 
// commands when the GUI opens and closes communications with us.
//
//----------------------------------------------------------------------
NTSTATUS AMDDispatch( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PIO_STACK_LOCATION      irpStack;
    PVOID                   inputBuffer;
    PVOID                   outputBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    ULONG                   ioControlCode;
    PSTORE_BUF              old;
    WORK_QUEUE_ITEM         workItem;
    IO_STATUS_BLOCK ioStatus;
    //
    // Go ahead and set the request up as successful
    //
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the pointer to the input/output buffer and its length
    //
    inputBuffer             = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength       = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBuffer            = Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength      = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode           = irpStack->Parameters.DeviceIoControl.IoControlCode;


    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:
        Sequence = 0;
        GUIActive = TRUE;
        break;

    case IRP_MJ_SHUTDOWN:
         while( old = AMDOldestStore()) {
            if( old == Store ) break;
			}
        break;

    case IRP_MJ_CLOSE:
        GUIActive = FALSE;
        AMDResetStore();
        break;

    case IRP_MJ_DEVICE_CONTROL:
        if( IOCTL_TRANSFER_TYPE(ioControlCode) == METHOD_NEITHER ) {
			outputBuffer = Irp->UserBuffer;
			}
        AMDDeviceControl( irpStack->FileObject, TRUE,
                             inputBuffer, inputBufferLength, 
                             outputBuffer, outputBufferLength,
                             ioControlCode, &Irp->IoStatus, DeviceObject );
        break;
    }
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;   
}


//----------------------------------------------------------------------
//
// AMDUnload
//
// Our job is done - time to leave.
//
//----------------------------------------------------------------------
VOID AMDUnload( IN PDRIVER_OBJECT DriverObject )
{
    WCHAR                   deviceLinkBuffer[]  = L"\\DosDevices\\AMDDRV";
    UNICODE_STRING          deviceLinkUnicodeString;


    //
    // Unhook the registry
    //
    if( RegHooked ) UnHookZWRoutines();
    //
    // Delete the symbolic link for our device
    //
    RtlInitUnicodeString( &deviceLinkUnicodeString, deviceLinkBuffer );
    IoDeleteSymbolicLink( &deviceLinkUnicodeString );

    //
    // Delete the device object
    //
    IoDeleteDevice( DriverObject->DeviceObject );


    //
    // Now we can free any memory we have outstanding
    //
    AMDFreeStore();

}

//----------------------------------------------------------------------
//
// DriverEntry
//
// Installable driver initialization. Here we just set ourselves up.
//
//----------------------------------------------------------------------
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
    NTSTATUS                ntStatus;
    WCHAR                   deviceNameBuffer[]  = L"\\Device\\AMDFDRV";
    UNICODE_STRING          deviceNameUnicodeString;
    WCHAR                   deviceLinkBuffer[]  = L"\\DosDevices\\AMDFDRV";
    UNICODE_STRING          deviceLinkUnicodeString;    
    WCHAR                   startValueBuffer[] = L"Start";
    UNICODE_STRING          startValueUnicodeString;
    WCHAR                   bootMessage[] = 
        L"\nAMD is logging Registry activity to \\SystemRoot\\AMDF.log\n\n";
    UNICODE_STRING          bootMessageUnicodeString;
    UNICODE_STRING          registryPath; 
    HANDLE                  driverKey;
    PETHREAD                curthread;
    ULONG                   startType, demandStart;
    RTL_QUERY_REGISTRY_TABLE paramTable[2]; 
    OBJECT_ATTRIBUTES       objectAttributes;
    int                     i;

    // 
    // Query our start type to see if we are supposed to monitor starting
    // at boot time
    //
    registryPath.Buffer = ExAllocatePool( PagedPool, 
                                          RegistryPath->Length + sizeof(UNICODE_NULL)); 
 
    if (!registryPath.Buffer) { 
 
        return STATUS_INSUFFICIENT_RESOURCES; 
    } 
 
    registryPath.Length = RegistryPath->Length + sizeof(UNICODE_NULL); 
    registryPath.MaximumLength = registryPath.Length; 

    RtlZeroMemory( registryPath.Buffer, registryPath.Length ); 
 
    RtlMoveMemory( registryPath.Buffer,  RegistryPath->Buffer, 
                   RegistryPath->Length  ); 

    RtlZeroMemory( &paramTable[0], sizeof(paramTable)); 
    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT; 
    paramTable[0].Name = L"Start"; 
    paramTable[0].EntryContext = &startType;
    paramTable[0].DefaultType = REG_DWORD; 
    paramTable[0].DefaultData = &startType; 
    paramTable[0].DefaultLength = sizeof(ULONG); 

    RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                            registryPath.Buffer, &paramTable[0], 
                            NULL, NULL  );


    //
    // Setup our name and symbolic link
    //
    RtlInitUnicodeString (&deviceNameUnicodeString,deviceNameBuffer );
    RtlInitUnicodeString (&deviceLinkUnicodeString,deviceLinkBuffer );

    //
    // Set up the device used for GUI communications
    //
    ntStatus = IoCreateDevice ( DriverObject,
                                0,
                                &deviceNameUnicodeString,
                                FILE_DEVICE_AMDDRV,
                                0,
                                TRUE,
                                &GUIDevice );
    if (NT_SUCCESS(ntStatus)) {
                
        //
        // Create a symbolic link that the GUI can specify to gain access
        // to this driver/device
        //
        ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,&deviceNameUnicodeString );

        //
        // Create dispatch points for all routines that must be handled
        //
        DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]        =
        DriverObject->MajorFunction[IRP_MJ_CREATE]          =
        DriverObject->MajorFunction[IRP_MJ_CLOSE]           =
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = AMDDispatch;
        // Create dispatch points for all routines that must be handled. 
        // All entry points are registered since we might filter a
        // file system that processes all of them.
        //
#if DBG

        DriverObject->DriverUnload                          = AMDUnload;
#endif
    }
    if (!NT_SUCCESS(ntStatus)) {


        //
        // Something went wrong, so clean up (free resources etc)
        //
        if( GUIDevice ) IoDeleteDevice( GUIDevice );
        IoDeleteSymbolicLink( &deviceLinkUnicodeString );
        return ntStatus;
    }

    //
    // Initialize our mutexes
    //
    MUTEX_INIT( StoreMutex );
    MUTEX_INIT( HashMutex );
    MUTEX_INIT( FilterMutex );

    //
    // Set up the Fast I/O dispatch table
    //
    //DriverObject->FastIoDispatch = &FastIOHook;
    //
    // Initialize the name hash table
    //
    for(i = 0; i < NUMHASH; i++ ) HashTable[i] = NULL;

    //
    // Pointer to system table data structure is an NTOSKRNL export
    //
    ServiceTable = KeServiceDescriptorTable;

    //
    // Find the process name offset
    //
    ProcessNameOffset = GetProcessNameOffset();

    //
    // Allocate the initial output buffer
    //
    Store   = ExAllocatePool( PagedPool, sizeof(*Store) );
    if ( !Store ) {

        IoDeleteDevice( GUIDevice );
        IoDeleteSymbolicLink( &deviceLinkUnicodeString );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Store->Len  = 0;
    Store->Next = NULL;
    NumStore = 1;

	//AMDFDRV_OpenLog();
    return STATUS_SUCCESS;
}



//----------------------------------------------------------------------
//
// AMDGetUOpenEffectiveToken
//
// Opens a handle to the current thread's effective token (either
// impersonation token or primary token).
//
//----------------------------------------------------------------------
HANDLE 
AMDGetUOpenEffectiveToken(VOID)
{
    NTSTATUS     status;
    PVOID        token;
    HANDLE       tokenHandle;
    BOOLEAN      tokenCopy;
    BOOLEAN      tokenEffective;
    SECURITY_IMPERSONATION_LEVEL tokenLevel;

    token = PsReferenceImpersonationToken( 
                          PsGetCurrentThread(), 
                          &tokenCopy, 
                          &tokenEffective, 
                          &tokenLevel 
                          );

    if( !token ) {
        token = PsReferencePrimaryToken( 
                          PsGetCurrentProcess()
                          );

    }

    //
    // Now that we have a token reference, get a handle to it
    // so that we can query it.
    //
    status = ObOpenObjectByPointer( token,0, NULL,TOKEN_QUERY, NULL, KernelMode, &tokenHandle );

    ObDereferenceObject( token );
    if( !NT_SUCCESS( status )) {

        //
        // We coudn't open the token!!
        //
        return NULL;
    }    
    return tokenHandle;
}

//----------------------------------------------------------------------
//
// AMDGetUGetLogonId
//
// Given a token handle, retreives the logonId.
//
//----------------------------------------------------------------------
//
// System account strings
//

BOOLEAN AMDGetUGetLogonId(HANDLE TokenHandle, char *UserName)
{
    NTSTATUS     status,rtlstat;
    ULONG        requiredLength;
    PSecurityUserData userInformation = NULL;
	int x;
	unsigned char *c,*p;
	char buffer[1024];
    PTOKEN_USER	tokenStats=(PTOKEN_USER)buffer;

    RtlZeroMemory( buffer, 1024); 

    //
    // Get the logon ID from the token
    //
    status = ZwQueryInformationToken( TokenHandle, 
                                      TokenUser,
                                      buffer,
                                      1024,
                                      &requiredLength 
                                      );
	// Convert SID to Char string
	p=UserName;
	c=(unsigned char *)tokenStats->User.Sid;

	for(x=0;x<28;x++) {
		sprintf(p,"%.2X",*(c));
		c++; p=p+2;
		}


    return STATUS_SUCCESS;

}

//----------------------------------------------------------------------
//
// AMDDRV_OpenLog
//
// Open a log file.
//
//----------------------------------------------------------------------
NTSTATUS AMDFDRV_OpenLog()
{
OBJECT_ATTRIBUTES       objectAttributes;
IO_STATUS_BLOCK         ioStatus;
NTSTATUS                ntStatus;
FILE_END_OF_FILE_INFORMATION zeroLengthFile;
ANSI_STRING  pastr;


	if(LogFile!=INVALID_HANDLE_VALUE) ZwClose( LogFile );
	RtlInitAnsiString(&pastr,lname);							// Make it ANSI_STRING
	RtlAnsiStringToUnicodeString(&LogFileName,&pastr,TRUE);		// Make it UNICODE_STRING

    InitializeObjectAttributes( &objectAttributes, &LogFileName,OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );

    ntStatus = ZwCreateFile( &LogFile, 
							 GENERIC_WRITE | GENERIC_READ,
                             &objectAttributes, 
							 &ioStatus, 
							 0, 
                             FILE_ATTRIBUTE_NORMAL, 
							 FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OVERWRITE_IF, 
							 FILE_SYNCHRONOUS_IO_NONALERT, 
							 NULL, 
							 0 );
    if (!NT_SUCCESS(ntStatus)) {
		UpdateStore( ":000000000000000:TrackFileLogError;%d;%sTIME(0:0)",ntStatus,lname);
		return FALSE;
		}
    zeroLengthFile.EndOfFile.QuadPart = 0;
    ZwSetInformationFile( LogFile, &ioStatus,&zeroLengthFile, sizeof(zeroLengthFile), FileEndOfFileInformation );
	ZwClose(LogFile);
	LogFile=INVALID_HANDLE_VALUE;
    return ntStatus;
}


void writetofile(PWSTR pfl,char *addr,int nsize)
{
   IO_STATUS_BLOCK IoStatus;
   OBJECT_ATTRIBUTES objectAttributes;
   HANDLE FileHandle = NULL;
   UNICODE_STRING fileName1;
   NTSTATUS status;
   char NL[2]={0x0d,0x0a};
   #define NLLEN	2


        fileName1.Buffer = NULL;
        fileName1.Length = 0;
        fileName1.MaximumLength = 256;
 
        fileName1.Buffer = (unsigned short *)ExAllocatePool(PagedPool,
        fileName1.MaximumLength);


        RtlZeroMemory(fileName1.Buffer, fileName1.MaximumLength);
        status = RtlAppendUnicodeToString(&fileName1, pfl);
        InitializeObjectAttributes (&objectAttributes,(PUNICODE_STRING)&fileName1,OBJ_CASE_INSENSITIVE,NULL,NULL );

       status = ZwCreateFile(&FileHandle,
        FILE_APPEND_DATA,
        &objectAttributes,
        &IoStatus,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0 );

       if(NT_SUCCESS(status)) {
        ZwWriteFile(FileHandle,NULL,NULL,NULL,&IoStatus,(void *)addr,nsize,NULL,NULL );
		nsize=2;
        ZwWriteFile(FileHandle,NULL,NULL,NULL,&IoStatus,(void *)NL,nsize,NULL,NULL );
        ZwClose(FileHandle);
		}

       if(fileName1.Buffer)
       ExFreePool(fileName1.Buffer);
}

/*****************************************************************************/
//                 Hook the Fast IO call for networked drives                //
/*****************************************************************************/
