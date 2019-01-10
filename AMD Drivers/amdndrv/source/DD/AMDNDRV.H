//======================================================================
// 
// AMDnDrv.h
//
//======================================================================
#include "wintypes.h"

//
// The name of the System process, in which context we're called in our 
//
#define SYSNAME    "System"

//
// Final Build numbers
//
#define NT4FINAL        1381
#define WIN2KFINAL      2195

//
// Maximum amount of memory grabbed 
//
#define MAXMEMORY       1000000

//
// Maximum path length of pathname. This is larger than Win32 maxpath
// because network drives have leading paths
//
#define MAXPATHLEN      1024

//
// Length of process name (rounded up to next DWORD)
//
#define PROCNAMELEN     32

//
// Max length of NT process name
//
#define NT_PROCNAMELEN  16

//
// Maximum seperate filter components 
//
#define MAXFILTERS      64
//
// Max size of a char SID
//
#define SIDCHARSIZE		256
//
// Length of buffer for error string
//
#define ERRORLEN        64

//
// Documented in the IFS Kit
//
typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG FileSystemAttributes;
    LONG MaximumComponentNameLength;
    ULONG FileSystemNameLength;
    WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;


//
// Structure for device specific data that keeps track of what
// drive and what filesystem device are hooked 
//
typedef struct {
    FILE_SYSTEM_TYPE Type;
    PDEVICE_OBJECT   FileSystem;
    unsigned         LogicalDrive;
    BOOLEAN          Hooked;
    PFILE_FS_ATTRIBUTE_INFORMATION FsAttributes;
} HOOK_EXTENSION, *PHOOK_EXTENSION;         


//
// Structure for the fileobject/name hash table
//
typedef struct _nameentry {
   PFILE_OBJECT		FileObject;
   struct _nameentry 	*Next;
   CHAR		FullPathName[];
} HASH_ENTRY, *PHASH_ENTRY;

//
// Structure for a completion routine work item
//
typedef struct _AMDnDrvwork {
    WORK_QUEUE_ITEM WorkItem;
    PFILE_OBJECT   FileObject;
    CHAR           ErrString[ERRORLEN];
} AMDnDrv_WORK, *PAMDnDrv_WORK;


//
// Number of hash buckets in the hash table
//
#define NUMHASH		0x100

//
// Hash function. Basically chops the low few bits of the file object
//
#if defined(_IA64_) 
#define HASHOBJECT(_fileobject)		(((ULONG_PTR)_fileobject)>>5)%NUMHASH
#else
#define HASHOBJECT(_fileobject)		(((ULONG)_fileobject)>>5)%NUMHASH
#endif


//
// Structure for keeping linked lists of output buffers
//
typedef struct _log {
    ULONG           Len;
    struct _log   * Next;
    CHAR            Data[ LOGBUFSIZE ];
} LOG_BUF, *PLOG_BUF;


//
// A check to see if a fastio table extends to a specific entry
//
#if defined(_IA64_) 
#define FASTIOPRESENT( _hookExt, _call )                                                      \
    (_hookExt->Type != GUIINTERFACE &&                                                        \
     _hookExt->FileSystem->DriverObject->FastIoDispatch &&                                    \
     (((ULONG_PTR)&_hookExt->FileSystem->DriverObject->FastIoDispatch->_call -                    \
       (ULONG_PTR) &_hookExt->FileSystem->DriverObject->FastIoDispatch->SizeOfFastIoDispatch <    \
       (ULONG_PTR) _hookExt->FileSystem->DriverObject->FastIoDispatch->SizeOfFastIoDispatch )) && \
      _hookExt->FileSystem->DriverObject->FastIoDispatch->_call )
#else
#define FASTIOPRESENT( _hookExt, _call )                                                      \
    (_hookExt->Type != GUIINTERFACE &&                                                        \
     _hookExt->FileSystem->DriverObject->FastIoDispatch &&                                    \
     (((ULONG)&_hookExt->FileSystem->DriverObject->FastIoDispatch->_call -                    \
       (ULONG) &_hookExt->FileSystem->DriverObject->FastIoDispatch->SizeOfFastIoDispatch <    \
       (ULONG) _hookExt->FileSystem->DriverObject->FastIoDispatch->SizeOfFastIoDispatch )) && \
      _hookExt->FileSystem->DriverObject->FastIoDispatch->_call )
#endif

//
// Time stamp start macro
//
#define TIMESTAMPSTART()                                  \
        timeStampStart = KeQueryPerformanceCounter(NULL); \
        KeQuerySystemTime( &dateTime )

#define TIMESTAMPSTOP()                                      \
        timeStampComplete = KeQueryPerformanceCounter(NULL); \
        timeResult.QuadPart = timeStampComplete.QuadPart - timeStampStart.QuadPart; 

//
// Macro for getting the path name
//
#define GETPATHNAME(_IsCreate)                                                  \
        fullPathName = ExAllocateFromNPagedLookasideList( &FullPathLookaside ); \
        if( fullPathName ) {							                        \
			RtlZeroMemory( fullPathName, 256 );									\
            AMDnDrvGetFullPath( _IsCreate, FileObject, hookExt, fullPathName ); \
        } else {                                                                \
            fullPathName = InsufficientResources;                               \
        }                                                                                    

#define FREEPATHNAME()                                   \
        if ( fullPathName != InsufficientResources ) ExFreeToNPagedLookasideList( &FullPathLookaside, fullPathName )

HANDLE AMDGetUOpenEffectiveToken(VOID);
BOOLEAN AMDGetUGetLogonId(HANDLE TokenHandle, char *UserName);
int ConvertToUpper( PCHAR Dest, PCHAR Source, ULONG Len );
int ExcludeProcess(char *name);
int IncludePath(char *name);
int ExcludePath(char *name);

typedef PVOID           POBJECT;

NTSTATUS ObOpenObjectByPointer(POBJECT Object, ULONG HandleAttributes, 
							    PACCESS_STATE PassedAccessState, ACCESS_MASK DesiredAccess, 
                                POBJECT_TYPE ObjectType, 
                                KPROCESSOR_MODE AccessMode, HANDLE *Handle );
NTSTATUS ZwQueryInformationToken (HANDLE TokenHandle,
                                  TOKEN_INFORMATION_CLASS TokenInformationClass,
                                  PVOID TokenInformation,
                                  ULONG TokenInformationLength,
                                  PULONG ReturnLength );