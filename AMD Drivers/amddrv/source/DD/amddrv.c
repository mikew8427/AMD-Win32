//----------------------------------------------------------------------
// Kernel Mode Driver to hook the registry. 
//----------------------------------------------------------------------
#include "ntddk.h"
#include "ksecdd.h"
#include "stdarg.h"
#include "stdio.h"
#include "ntsec.h"
#include "ioctlcmd.h"
#include "amddrv.h"

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

//
// Macro for Hooking The NT Service table
//

#if defined(_ALPHA_)
#define SYSCALL(_function)  ServiceTable->ServiceTable[ (*(PULONG)_function)  & 0x0000FFFF ]
#else
#define SYSCALL(_function)  ServiceTable->ServiceTable[ *(PULONG)((PUCHAR)_function+1)]
#endif

//
// Number of predefined rootkeys
//
#define NUMROOTKEYS     4


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
NTSTATUS AMDSDispatch( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
VOID     AMDSUnload( IN PDRIVER_OBJECT DriverObject );

//----------------------------------------------------------------------
//                         GLOBALS
//---------------------------------------------------------------------- 
// our user-inteface device object
PDEVICE_OBJECT          AMDRDev;

//
// Is a GUI talking to us?
//
BOOLEAN                 SvcRunning = FALSE;

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

VOID ConvertToUpper( PCHAR Dest, PCHAR Source, ULONG Len );
#define _MAX_FILENAME	512
//
// We save off pointers to the actual Registry functions in these variables
//
NTSTATUS (*RealRegOpenKey)( IN PHANDLE, IN OUT ACCESS_MASK, IN POBJECT_ATTRIBUTES );
NTSTATUS (*RealRegQueryKey)( IN HANDLE, IN KEY_INFORMATION_CLASS,OUT PVOID, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegQueryValueKey)( IN HANDLE, IN PUNICODE_STRING,IN KEY_VALUE_INFORMATION_CLASS,OUT PVOID, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegEnumerateValueKey)( IN HANDLE, IN ULONG,IN KEY_VALUE_INFORMATION_CLASS,OUT PVOID, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegEnumerateKey)( IN HANDLE, IN ULONG,IN KEY_INFORMATION_CLASS,OUT PVOID, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegSetValueKey)( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName,IN ULONG TitleIndex, IN ULONG Type,IN PVOID Data, IN ULONG DataSize );
NTSTATUS (*RealRegCreateKey)( OUT PHANDLE, IN ACCESS_MASK,IN POBJECT_ATTRIBUTES , IN ULONG,IN PUNICODE_STRING, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegDeleteValueKey)( IN HANDLE, IN PUNICODE_STRING );
NTSTATUS (*RealRegCloseKey)( IN HANDLE );
NTSTATUS (*RealRegDeleteKey)( IN HANDLE );
NTSTATUS (*RealRegFlushKey)( IN HANDLE );
NTSTATUS (*RealCreateSection) (OUT PHANDLE, IN ULONG,IN POBJECT_ATTRIBUTES OPTIONAL,IN PLARGE_INTEGER OPTIONAL,
							   IN ULONG, IN ULONG, IN HANDLE OPTIONAL);
//
// Lenghs of rootkeys (filled in at init). This table allows us to translate 
// path names into better-known forms. Current user is treated specially since
// its not a full match.
//
ROOTKEY CurrentUser[2] = {
    { "\\\\REGISTRY\\USER\\S", "HKCU", 0 },
    { "HKU\\S", "HKCU", 0 }
};

ROOTKEY RootKey[NUMROOTKEYS] = {
    { "\\\\REGISTRY\\USER", "HKU", 0 },
    { "\\\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT", 
      "HKCC", 0 },
    { "\\\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES", "HKCR", 0 },
    { "\\\\REGISTRY\\MACHINE", "HKLM", 0 }
};

//
// This is a hash table for keeping names around for quick lookup.
//
PHASH_ENTRY             HashTable[NUMHASH];

//
// Mutex for hash table accesses
//
KMUTEX                  HashMutex;

//
// Data structure for storing messages we generate
//
PSTORE_BUF              Store           = NULL;
ULONG                   Sequence        = 0;
KMUTEX                  StoreMutex;

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

BOOLEAN AMDGetLogonId(HANDLE TokenHandle, char *UserName);
HANDLE AMDOpenEffectiveToken(VOID);




//======================================================================
//                   B U F F E R  R O U T I N E S 
//======================================================================

//----------------------------------------------------------------------
//
// AMDSFreeStore
//
// Frees all the data output buffers that we have currently allocated.
//
//----------------------------------------------------------------------
VOID AMDSFreeStore()
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
// AMDSNewStore
//
// Called when the current buffer has filled up. This moves us to the
// pre-allocated buffer and then allocates another buffer.
//
//----------------------------------------------------------------------
void AMDSNewStore( void )
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
		} 
	else {
        Store->Len = 0;
		}
}


//----------------------------------------------------------------------
//
// AMDSOldestStore
//
// Goes through the linked list of storage buffers and returns the 
// oldest one.
//
//----------------------------------------------------------------------
PSTORE_BUF AMDSOldestStore( void )
{
    PSTORE_BUF  ptr = Store, prev = NULL;

    while ( ptr->Next ) {
        ptr = (prev = ptr)->Next;
		}
    if ( prev ) prev->Next = NULL;    
    
	NumStore--;
    return ptr;
}


//----------------------------------------------------------------------
//
// AMDSResetStore
//
// When a GUI is no longer communicating with us, but we can't unload,
// we reset the storage buffers.
//
//----------------------------------------------------------------------
VOID AMDSResetStore()
{
    PSTORE_BUF  current, next;

    MUTEX_WAIT( StoreMutex );

    //
    // Traverse the list of output buffers
    //
    current = Store->Next;
    while( current ) {
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
	len += 4; len &=  0xFFFFFFFC; // +1 to include null terminator and +3 to allign on longword

    //
    // See if its time to switch to extra buffer
    //
    if ( Store->Len + len + sizeof(ENTRY) + 1 >= MAX_STORE ) {
        AMDSNewStore();
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
//                   H A S H  R O U T I N E S 
//======================================================================

//----------------------------------------------------------------------
//
// AMDSHashCleanup
//
// Called when we are unloading to free any memory that we have 
// in our possession.
//
//----------------------------------------------------------------------
VOID AMDSHashCleanup()
{
    PHASH_ENTRY             hashEntry, nextEntry;
    ULONG                   i;

    MUTEX_WAIT( HashMutex );    

    //
    // First free the hash table entries
    //       
    for( i = 0; i < NUMHASH; i++ ) {
        hashEntry = HashTable[i];
        while( hashEntry ) {
            nextEntry = hashEntry->Next;
            ExFreePool( hashEntry->FullPathName );
            ExFreePool( hashEntry );
            hashEntry = nextEntry;
        }
    }
    
    hashEntry = FreeHashList;
    while( hashEntry ) {
        nextEntry = hashEntry->Next;
        ExFreePool( hashEntry );
        hashEntry = nextEntry;
    }
    MUTEX_RELEASE( HashMutex );
}

//----------------------------------------------------------------------
//
// AMDSStoreHash
//
// Stores the key and associated fullpath in the hash table.
//
//----------------------------------------------------------------------
VOID AMDSStoreHash( POBJECT object, PCHAR fullname )
{
    PHASH_ENTRY     newEntry;

    MUTEX_WAIT( HashMutex );

    if( FreeHashList ) {
        newEntry = FreeHashList;
        FreeHashList = newEntry->Next;
		} 
	else {
        newEntry = ExAllocatePool( PagedPool, sizeof(HASH_ENTRY));
		}

    newEntry->Object                = object;
    newEntry->FullPathName          = ExAllocatePool( PagedPool, strlen(fullname)+1 );
    newEntry->Next                  = HashTable[ HASHOBJECT( object) ];
    HashTable[ HASHOBJECT(object) ] = newEntry;     
    strcpy( newEntry->FullPathName, fullname );

    MUTEX_RELEASE( HashMutex );
}


//----------------------------------------------------------------------
//
// AMDSFreeHashEntry
//
// When we see a key close, we can free the string we had associated
// with the fileobject being closed since we know it won't be used
// again.
//
//----------------------------------------------------------------------
VOID AMDSFreeHashEntry( POBJECT object )
{
    PHASH_ENTRY             hashEntry, prevEntry;

    MUTEX_WAIT( HashMutex );

    //
    // look-up the entry
    //
    hashEntry = HashTable[ HASHOBJECT( object ) ];
    prevEntry = NULL;
    while( hashEntry && hashEntry->Object != object ) {
        prevEntry = hashEntry;
        hashEntry = hashEntry->Next;
		}
  
    //
    // If we fall off (didn''t find it), just return
    //
    if( !hashEntry ) {
        MUTEX_RELEASE( HashMutex );
        return;
		}

    //
    // Remove it from the hash list 
    //
    if( prevEntry ) prevEntry->Next = hashEntry->Next;
    else HashTable[ HASHOBJECT( object )] = hashEntry->Next;

    //
    // Free the memory associated with it
    //
    ExFreePool( hashEntry->FullPathName );
    hashEntry->Next = FreeHashList;
    FreeHashList = hashEntry;

    MUTEX_RELEASE( HashMutex );
}

//----------------------------------------------------------------------
//
// ConverToUpper
//
// Obvious.
//
//----------------------------------------------------------------------
VOID ConvertToUpper( PCHAR Dest, PCHAR Source, ULONG Len )
{
    ULONG   i;
    
    for( i = 0; i < Len; i++ ) {
        if( Source[i] >= 'a' && Source[i] <= 'z' ) {
            Dest[i] = Source[i] - 'a' + 'A';
			} 
		else {
            Dest[i] = Source[i];
			}
		}
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
// AppendKeyInformation
//
// Appends key enumerate and query information to the output buffer.
//
//----------------------------------------------------------------------
VOID AppendKeyInformation( IN KEY_INFORMATION_CLASS KeyInformationClass,
                           IN PVOID KeyInformation, PCHAR Buffer )
{
    PKEY_BASIC_INFORMATION  pbasicinfo;
    PKEY_FULL_INFORMATION   pfullinfo;
    PKEY_NODE_INFORMATION   pnodeinfo;
    UNICODE_STRING          ukeyname;       
    ANSI_STRING             akeyname;

    switch( KeyInformationClass ) {

    case KeyBasicInformation:
        pbasicinfo = (PKEY_BASIC_INFORMATION) KeyInformation;
        ukeyname.Length = (USHORT) pbasicinfo->NameLength;
        ukeyname.MaximumLength = (USHORT) pbasicinfo->NameLength;
        ukeyname.Buffer = pbasicinfo->Name;
        RtlUnicodeStringToAnsiString( &akeyname, &ukeyname, TRUE );
        sprintf( Buffer, "Name: %s", akeyname.Buffer );
        RtlFreeAnsiString( &akeyname );
        break;

    case KeyFullInformation:
        pfullinfo = (PKEY_FULL_INFORMATION) KeyInformation;
        sprintf( Buffer, "Subkeys = %d", pfullinfo->SubKeys );
        break;  
        
    case KeyNodeInformation:
        pnodeinfo = (PKEY_NODE_INFORMATION) KeyInformation;
        ukeyname.Length = (USHORT) pnodeinfo->NameLength;
        ukeyname.MaximumLength = (USHORT) pnodeinfo->NameLength;
        ukeyname.Buffer = pnodeinfo->Name;
        RtlUnicodeStringToAnsiString( &akeyname, &ukeyname, TRUE );
        sprintf( Buffer, "Name: %s", akeyname.Buffer );
        RtlFreeAnsiString( &akeyname );
        break;

    default:
        sprintf( Buffer, "Unknown Info Class" );
        break;
    }
}


//----------------------------------------------------------------------
//
// AppendRegValueType
//
// Returns the string form of an registry value type.
//
//----------------------------------------------------------------------
VOID AppendRegValueType( ULONG Type, PCHAR Buffer )
{
    CHAR            tmp[MAXDATALEN];

    switch( Type ) {
    case REG_BINARY:
        strcat( Buffer, "BINARY" );
        break;
    case REG_DWORD_LITTLE_ENDIAN:
        strcat( Buffer, "DWORD_LITTLE_END" );
        break;
    case REG_DWORD_BIG_ENDIAN:
        strcat( Buffer, "DWORD_BIG_END" );
        break;
    case REG_EXPAND_SZ:
        strcat( Buffer, "EXPAND_SZ" );
        break;
    case REG_LINK:
        strcat( Buffer, "LINK" );
        break;
    case REG_MULTI_SZ:
        strcat( Buffer, "MULTI_SZ" );
        break;
    case REG_NONE:
        strcat( Buffer, "NONE" );
        break;
    case REG_SZ:
        strcat( Buffer, "SZ" );
        break;
    case REG_RESOURCE_LIST:
        strcat( Buffer, "RESOURCE_LIST" );
        break;
    case REG_RESOURCE_REQUIREMENTS_LIST:
        strcat( Buffer, "REQ_LIST" );
        break;
    case REG_FULL_RESOURCE_DESCRIPTOR:
        strcat( Buffer, "DESCRIPTOR" );
        break;
    default:
        sprintf( tmp, "UNKNOWN TYPE: %d", Type );
        strcat( Buffer, tmp );
        break;
    }
}


//----------------------------------------------------------------------
//
// AppendRegValueData
//
//----------------------------------------------------------------------
VOID AppendRegValueData( IN ULONG Type, IN PVOID Data, IN ULONG Length, 
                         IN OUT PCHAR Buffer )
{
    PWCHAR                  pstring;
    PULONG                  pulong;
    PUCHAR                  pbinary;
    CHAR                    tmp[MAXDATALEN];
    UNICODE_STRING          ukeyname;       
    ANSI_STRING             akeyname;
    int                     len, i;

    switch( Type ) {
    case REG_SZ:    
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:
        pstring = (PWCHAR) Data;
        ukeyname.Length = (USHORT) Length;
        ukeyname.MaximumLength = (USHORT) Length;
        ukeyname.Buffer = pstring;
        RtlUnicodeStringToAnsiString( &akeyname, 
                                      &ukeyname, TRUE );    
        strcat( Buffer, "\"");
        strncatZ( Buffer+1, akeyname.Buffer, MAXVALLEN - 6);
        if( akeyname.Length > MAXVALLEN - 6 ) strcat( Buffer,"...");
        strcat( Buffer, "\"");
        RtlFreeAnsiString( &akeyname );
        break;

    case REG_DWORD:
        pulong = (PULONG) Data;
        sprintf( tmp, "0x%X", *pulong );
        strcat( Buffer, tmp );
        break;

    case REG_NONE:
    case REG_BINARY:
    case REG_RESOURCE_LIST:
    case REG_FULL_RESOURCE_DESCRIPTOR:
    case REG_RESOURCE_REQUIREMENTS_LIST:
        pbinary = (PCHAR) Data;
        if( Length > 64 ) len = 64;
        else len = Length;
        for( i = 0; i < len; i++ ) {
            sprintf( tmp, "%02X ", (UCHAR) pbinary[i]);
            strcat( Buffer, tmp );
        }
        if( Length > 64) strcat( Buffer, "...");
        break;

    default:
        AppendRegValueType( Type, Buffer );
        break;
    }
}


//----------------------------------------------------------------------
//
// AppendValueInformation
//
// Appends value enumerate and query information to the output buffer.
//
//----------------------------------------------------------------------
VOID AppendValueInformation( IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
                             IN PVOID KeyValueInformation, PCHAR Buffer, PCHAR ValueName )
{
    PKEY_VALUE_BASIC_INFORMATION    pbasicinfo;
    PKEY_VALUE_FULL_INFORMATION     pfullinfo;
    PKEY_VALUE_PARTIAL_INFORMATION  ppartinfo;
    UNICODE_STRING                  ukeyname;       
    ANSI_STRING                     akeyname;

    switch( KeyValueInformationClass ) {

    case KeyValueBasicInformation:
        pbasicinfo = (PKEY_VALUE_BASIC_INFORMATION)
            KeyValueInformation;
        sprintf( Buffer, "Type: ");
        AppendRegValueType( pbasicinfo->Type, Buffer );
        strncatZ( Buffer, " Name: ", MAXVALLEN - 1 - strlen(Buffer) );
        ukeyname.Length = (USHORT) pbasicinfo->NameLength;
        ukeyname.MaximumLength = (USHORT) pbasicinfo->NameLength;
        ukeyname.Buffer = pbasicinfo->Name;
        RtlUnicodeStringToAnsiString( &akeyname, &ukeyname, TRUE );
        strncatZ( Buffer, akeyname.Buffer, MAXVALLEN - 1 - strlen(Buffer) );
        if( ValueName ) strncpy( ValueName, akeyname.Buffer, MAXVALLEN - 1 );
        RtlFreeAnsiString( &akeyname );                 
        break;

    case KeyValueFullInformation:   
        pfullinfo = (PKEY_VALUE_FULL_INFORMATION) 
            KeyValueInformation;
        AppendRegValueData( pfullinfo->Type, 
                            (PVOID) ((PCHAR) pfullinfo + pfullinfo->DataOffset), 
                            pfullinfo->DataLength, Buffer );
        if( ValueName ) {
            ukeyname.Length = (USHORT) pfullinfo->NameLength;
            ukeyname.MaximumLength = (USHORT) pfullinfo->NameLength;
            ukeyname.Buffer = pfullinfo->Name;
            RtlUnicodeStringToAnsiString( &akeyname, &ukeyname, TRUE );
            strncpy( ValueName, akeyname.Buffer, MAXVALLEN - 1 );
            RtlFreeAnsiString( &akeyname ); 
        }
        break;

    case KeyValuePartialInformation:
        ppartinfo = (PKEY_VALUE_PARTIAL_INFORMATION)
            KeyValueInformation;
        AppendRegValueData( ppartinfo->Type, 
                            (PVOID) ppartinfo->Data, 
                            ppartinfo->DataLength, Buffer );
        break;

    default:
        sprintf( Buffer, "Unknown Info Class" );
        break;
    }
}

//----------------------------------------------------------------------
//
// ErrorString
//
// Returns the string form of an error code.
//
//----------------------------------------------------------------------
PCHAR ErrorString( NTSTATUS retval )
{
    //
    // Before transating, apply error filter
    //
    if( retval == STATUS_SUCCESS && !FilterDef.logsuccess ) return NULL;
    if( retval != STATUS_SUCCESS && !FilterDef.logerror   ) return NULL;

    //
    // Passed filter, so log it
    //
    switch( retval ) {
    case STATUS_BUFFER_TOO_SMALL:
        return "BUFTOOSMALL";
    case STATUS_SUCCESS:
        return "SUCCESS";
    case STATUS_KEY_DELETED:
        return "KEYDELETED";
    case STATUS_REGISTRY_IO_FAILED:
        return "IOFAILED";
    case STATUS_REGISTRY_CORRUPT:
        return "CORRUPT";
    case STATUS_NO_MEMORY:
        return "OUTOFMEM";
    case STATUS_ACCESS_DENIED:
        return "ACCDENIED";
    case STATUS_NO_MORE_ENTRIES:
        return "NOMORE";
    case STATUS_OBJECT_NAME_NOT_FOUND:
        return "NOTFOUND";
    case STATUS_BUFFER_OVERFLOW:
        return "BUFOVRFLOW";
    case STATUS_OBJECT_PATH_SYNTAX_BAD:
        return "SYNTAXERR";
    case STATUS_ACCESS_VIOLATION:  
        return "ACCESS_ERROR";
    default:
        sprintf(errstring, "%x", retval );
        return errstring;
    }
}


//----------------------------------------------------------------------
//
// AMDSFreeFilters
//
// Fress storage we allocated for filter strings.
//
//----------------------------------------------------------------------
VOID AMDSFreeFilters()
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
// AMDSUpdateFilters
//
// Takes a new filter specification and updates the filter
// arrays with them.
//
//----------------------------------------------------------------------
VOID AMDSUpdateFilters()
{
    //
    // Free old filters (if any)
    //
    MUTEX_WAIT( FilterMutex );

    AMDSFreeFilters();

    //
    // Create new filter arrays
    //
    MakeFilterArray( FilterDef.processfilter,
                     ProcessFilters, &NumProcessFilters );
    MakeFilterArray( FilterDef.processexclude,
                     ProcessExcludeFilters, &NumProcessExcludeFilters );
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
		for(x=0, y=0;PathIncludeFilters[i][x] != '\0'; x++,y++)
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
// GetFullName
//
// Returns the full pathname of a key, if we can obtain one, else
// returns a handle.
//
//----------------------------------------------------------------------
void GetFullName( HANDLE hKey, PUNICODE_STRING lpszSubKeyVal, 
                  PCHAR fullname )
{
    PHASH_ENTRY             hashEntry;
    POBJECT                 pKey = NULL;
    CHAR                    tmpkey[16];
    ANSI_STRING             keyname;
    PCHAR                   tmpname;
    CHAR                    cmpname[MAXROOTLEN];
    PCHAR                   nameptr;
    PUNICODE_STRING         fullUniName;
    ULONG                   actualLen;
    int                     i;

    //
    // Allocate a temporary buffer
    //
    tmpname = ExAllocatePool( PagedPool, MAXPATHLEN );

    //
    // Translate the hkey into a pointer
    //
    fullname[0] = 0;
    tmpname[0] = 0;

    //
    // Is it a valid handle?
    //
    if( pKey = GetPointer( hKey )) {

        //
        // See if we find the key in the hash table
        //
        ReleasePointer( pKey );
        MUTEX_WAIT( HashMutex );
        hashEntry = HashTable[ HASHOBJECT( pKey ) ];
        while( hashEntry && hashEntry->Object != pKey )
            hashEntry = hashEntry->Next;
        MUTEX_RELEASE( HashMutex );

        if( hashEntry ) {

            strcpy( tmpname, hashEntry->FullPathName );

        } else {

            //
            // We will only get here if key was created before we loaded - ask the Configuration
            // Manager what the name of the key is.
            //
            if( pKey ) {

                fullUniName = ExAllocatePool( PagedPool, MAXPATHLEN*2+2*sizeof(ULONG));
                fullUniName->MaximumLength = MAXPATHLEN*2;
        
                if( NT_SUCCESS(ObQueryNameString( pKey, fullUniName, MAXPATHLEN, &actualLen ) )) {

                    RtlUnicodeStringToAnsiString( &keyname, fullUniName, TRUE ); 
                    if( keyname.Buffer[0] ) {         
                        strcpy( tmpname, "\\" );
                        strncatZ( tmpname, keyname.Buffer, MAXPATHLEN -2 );
                    }
                    RtlFreeAnsiString( &keyname );
                }
                ExFreePool( fullUniName );
            }
        }
    }

    //
    // Append subkey and value, if they are there
    //
    if( lpszSubKeyVal ) {
        RtlUnicodeStringToAnsiString( &keyname, lpszSubKeyVal, TRUE );
        if( keyname.Buffer[0] ) {
            strcat( tmpname, "\\" );
            strncatZ( tmpname, keyname.Buffer, MAXPATHLEN - 1 - strlen(tmpname) );
        }
        RtlFreeAnsiString( &keyname );
    }

    //
    // See if it matches current user
    //
    for( i = 0; i < 2; i++ ) {
        ConvertToUpper( cmpname, tmpname, CurrentUser[i].RootNameLen );
        if( !strncmp( cmpname, CurrentUser[i].RootName,
                      CurrentUser[i].RootNameLen )) {
            //
            // Its current user. Process to next slash
            //
            nameptr = tmpname + CurrentUser[i].RootNameLen;
            while( *nameptr && *nameptr != '\\' ) nameptr++;
            strcpy( fullname, CurrentUser[i].RootShort );
            strcat( fullname, nameptr );
            ExFreePool( tmpname );
            return;
        }
    }     

    //
    // Now, see if we can translate a root key name
    //
    for( i = 0; i < NUMROOTKEYS; i++ ) {
        ConvertToUpper( cmpname, tmpname, RootKey[i].RootNameLen );
        if( !strncmp( cmpname, RootKey[i].RootName, 
                      RootKey[i].RootNameLen )) {
            nameptr = tmpname + RootKey[i].RootNameLen;
            strcpy( fullname, RootKey[i].RootShort );
            strcat( fullname, nameptr );
            ExFreePool( tmpname );
            return;
        }
    }

    //
    // No translation
    //
    strcpy( fullname, tmpname );
    ExFreePool( tmpname ); 
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
    ULONG           i;

    //
    // We only try and get the name if we located the name offset
    //
    if( ProcessNameOffset ) {
    
        curproc = PsGetCurrentProcess();
        nameptr   = (PCHAR) curproc + ProcessNameOffset;
        strncpy( Name, nameptr, 16 );

    } else {
     
        strcpy( Name, "Unknown");
    }

    //
    // Apply process name filters
    //
    MUTEX_WAIT( FilterMutex );
    for( i = 0; i < NumProcessExcludeFilters; i++ ) {
        if( (strstr( Name , ProcessExcludeFilters[i] )) ) {
            MUTEX_RELEASE( FilterMutex );
            return NULL;
        }
    }
    for( i = 0; i < NumProcessFilters; i++ ) {
        if( (strstr( Name , ProcessFilters[i] )) ) {
            MUTEX_RELEASE( FilterMutex );
            return Name;
        }
    }
    MUTEX_RELEASE( FilterMutex );
    return NULL;
}

//======================================================================
//                    H O O K  R O U T I N E S
//======================================================================

//----------------------------------------------------------------------
//
// HookRegCreateKey
//
//----------------------------------------------------------------------
NTSTATUS HookRegCreateKey( OUT PHANDLE pHandle, IN ACCESS_MASK ReqAccess,
                           IN POBJECT_ATTRIBUTES pOpenInfo, IN ULONG TitleIndex,
                           IN PUNICODE_STRING Class, IN ULONG CreateOptions, OUT PULONG Disposition )
{
    NTSTATUS                ntstatus;
    CHAR                    fullname[MAXPATHLEN]="", data[MAXDATALEN], name[MAXPROCNAMELEN],matchname[MAXPATHLEN];
    LONG			    filter;
    CHAR 			    stat[MAXPATHLEN]="Supress CreateKey";
    HANDLE				tokenHandle;
    char				userName[SIDCHARSIZE];


    RtlZeroMemory( matchname, MAXPATHLEN ); 
    GetFullName( pOpenInfo->RootDirectory, pOpenInfo->ObjectName, fullname );
    ConvertToUpper( matchname, fullname, strlen(fullname) );
    return RealRegCreateKey( pHandle, ReqAccess, pOpenInfo, TitleIndex,Class, CreateOptions, Disposition );

    // Ask if there is a match -1 is not in list, 0 is excluded, 1 > has a match
    filter=ApplyNameFilter(matchname);	
    if( filter < 0 ) {						// Not found in Filter list Just do
    	ntstatus = RealRegCreateKey( pHandle, ReqAccess, pOpenInfo, TitleIndex,Class, CreateOptions, Disposition );
		return ntstatus;
		}
    if( (filter >= 0) && !FilterDef.createmode[filter]) {	// Found in list but no Steady State Management	
    	ntstatus = RealRegCreateKey( pHandle, ReqAccess, pOpenInfo, TitleIndex,Class, CreateOptions, Disposition );
		strcpy(stat,"CreateKey");
		}
    else ntstatus=STATUS_ACCESS_VIOLATION;

    data[0] = 0;
    GetProcess( name );
    if( FilterDef.logwrites && (filter>=0) ) {
	    tokenHandle = AMDOpenEffectiveToken();
		RtlZeroMemory( userName, SIDCHARSIZE );
		if(tokenHandle) AMDGetLogonId(tokenHandle,&userName[0]);
        UpdateStore( ":%s:%s;%s;%s;%s;%s", userName,name,stat,matchname, ErrorString( ntstatus ), data);
		}
    return ntstatus;
}

//----------------------------------------------------------------------
//
// HookRegDeleteKey
//
// Once we've deleted a key, we can remove its reference in the hash 
// table.
//
//----------------------------------------------------------------------
NTSTATUS HookRegDeleteKey( IN HANDLE Handle )
{
    NTSTATUS            ntstatus;
    CHAR                fullname[MAXPATHLEN]="", name[MAXPROCNAMELEN],matchname[MAXPATHLEN];
    CHAR 			    stat[MAXPATHLEN]="Supress DeleteKey";
    LONG			    filter;
    HANDLE				tokenHandle;
    char				userName[SIDCHARSIZE];

    GetFullName( Handle, NULL, fullname );
    RtlZeroMemory( matchname, MAXPATHLEN ); 
    ConvertToUpper( matchname, fullname, strlen(fullname) );
    // Ask if there is a match -1 is not in list, 0 is excluded, 1 > has a match
    filter=ApplyNameFilter(matchname);	
    if( filter < 0 ) {						// Not found in Filter list Just do
    		ntstatus = RealRegDeleteKey( Handle );
		return ntstatus;
		}
    if( (filter >= 0) && !FilterDef.createmode[filter]) {	// Found in list but no Steady State Management	
    		ntstatus = RealRegDeleteKey( Handle );
		strcpy(stat,"DeleteKey");
		}
    else ntstatus=STATUS_ACCESS_VIOLATION;

    GetProcess( name );
    if( FilterDef.logwrites && (filter>=0) ) {
	    tokenHandle = AMDOpenEffectiveToken();
		RtlZeroMemory( userName, SIDCHARSIZE );
		AMDGetLogonId(tokenHandle,&userName[0]);
        UpdateStore( ":%s:%s;%s;%s;%s;\"\"",userName,name, stat, matchname,ErrorString( ntstatus ));
        }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegDeleteValueKey
//
//----------------------------------------------------------------------
NTSTATUS HookRegDeleteValueKey( IN HANDLE Handle, PUNICODE_STRING Name )
{
    NTSTATUS                ntstatus;
    CHAR                    fullname[MAXPATHLEN]="", name[MAXPROCNAMELEN],matchname[MAXPATHLEN];
    CHAR 			    stat[MAXPATHLEN]="Supress DeleteValueKey";
    LONG			    filter;
    HANDLE				tokenHandle;
    char				userName[SIDCHARSIZE];

    GetFullName( Handle, Name, fullname );
    RtlZeroMemory( matchname, MAXPATHLEN ); 
    ConvertToUpper( matchname, fullname, strlen(fullname) );
    // Ask if there is a match -1 is not in list, 0 is excluded, 1 > has a match
    filter=ApplyNameFilter(matchname);	
    if( filter < 0 ) {						// Not found in Filter list Just do
    		ntstatus = RealRegDeleteValueKey( Handle, Name );
		return ntstatus;
		}
    if( (filter >= 0) && !FilterDef.mode[filter]) {	// Found in list but no Steady State Management	
    		ntstatus = RealRegDeleteValueKey( Handle, Name );
		strcpy(stat,"DeleteValueKey");
		}
    else ntstatus=STATUS_ACCESS_VIOLATION;

    GetProcess( name );
    if( FilterDef.logwrites && (filter>=0) ) {
	    tokenHandle = AMDOpenEffectiveToken();
		RtlZeroMemory( userName, SIDCHARSIZE );
		AMDGetLogonId(tokenHandle,&userName[0]);
        UpdateStore( ":%s:%s;%s;%s;%s;\"\"", userName,name, stat, matchname, ErrorString( ntstatus ));
		}
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegSetValueKey
//
//---------------------------------------------------------------------- 
NTSTATUS HookRegSetValueKey( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName,
                             IN ULONG TitleIndex, IN ULONG Type, IN PVOID Data, IN ULONG DataSize )
{
    NTSTATUS                ntstatus;
    LONG					filter=0;
    PUNICODE_STRING         valueName;
    CHAR 					stat[MAXPATHLEN]="Supress SetValue";
    CHAR                    fullname[MAXPATHLEN]="", data[MAXVALLEN], name[MAXPROCNAMELEN], matchname[MAXPATHLEN], keyname[MAXPATHLEN], holdname[MAXPATHLEN];
    HANDLE					tokenHandle;
    char					userName[SIDCHARSIZE];

    if( !ValueName || !ValueName->Length ) valueName = &DefaultValue;
    else						     	   valueName = ValueName;


    GetFullName( KeyHandle, valueName, fullname );
    RtlZeroMemory( matchname, MAXPATHLEN ); 
    RtlZeroMemory( data, MAXVALLEN); 
    RtlZeroMemory( name, MAXPROCNAMELEN); 
    ConvertToUpper( matchname, fullname, strlen(fullname) );

    // Ask if there is a match -1 is not in list, 0 is excluded, 1 > has a match
    filter=ApplyNameFilter(matchname);	
    if( filter < 0 ) {
	      ntstatus = RealRegSetValueKey( KeyHandle, ValueName, TitleIndex,Type, Data, DataSize );
		return ntstatus;
		}
    if( (filter >= 0) && !FilterDef.mode[filter]) {	
	      ntstatus = RealRegSetValueKey( KeyHandle, ValueName, TitleIndex,Type, Data, DataSize );
		strcpy(stat,"SetValue");
		}
    else ntstatus=STATUS_ACCESS_VIOLATION;

    AppendRegValueData( Type, Data, DataSize, data );
    GetProcess( name );
    if( FilterDef.logwrites && (filter>=0) ) {
	    tokenHandle = AMDOpenEffectiveToken();
		RtlZeroMemory( userName, SIDCHARSIZE );
		if(tokenHandle) AMDGetLogonId(tokenHandle,&userName[0]);
	    UpdateStore( ":%s:%s;%s;%s;%s;%s",userName,name,stat, matchname, ErrorString( ntstatus ), data);
		}

return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegistry
//
// Replaces entries in the system service table with pointers to
// our own hook routines. We save off the real routine addresses.
//
//----------------------------------------------------------------------
VOID HookRegistry( void )
{
    if( !RegHooked ) {
_asm
{
CLI //dissable interrupt
MOV EAX, CR0 //move CR0 register into EAX
AND EAX, NOT 10000H //disable WP bit 
MOV CR0, EAX //write register back
}

        RealRegDeleteKey = SYSCALL( ZwDeleteKey );
        SYSCALL( ZwDeleteKey ) = (PVOID) HookRegDeleteKey;

        RealRegSetValueKey = SYSCALL( ZwSetValueKey );
        SYSCALL( ZwSetValueKey ) = (PVOID) HookRegSetValueKey;

        RealRegCreateKey = SYSCALL( ZwCreateKey );
        SYSCALL( ZwCreateKey ) = (PVOID) HookRegCreateKey;

        RealRegDeleteValueKey = SYSCALL( ZwDeleteValueKey );
        SYSCALL( ZwDeleteValueKey ) = (PVOID) HookRegDeleteValueKey;
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
// UnhookRegistry
//
// Just copy the Rotin above and switch the addresses (if you dare)
//----------------------------------------------------------------------
VOID UnhookRegistry( )
{
    if( RegHooked ) {
    }
}

//======================================================================
//         D E V I C E - D R I V E R  R O U T I N E S 
//======================================================================

//----------------------------------------------------------------------
//
// AMDSDeviceControl
//
//----------------------------------------------------------------------
BOOLEAN  AMDSDeviceControl( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait,
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
        HookRegistry();
        break;

    case AMDDRV_unhook:
        UnhookRegistry();
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
            AMDSNewStore();

            // Fetch the oldest to give to user
            old = AMDSOldestStore();
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
        AMDSUpdateFilters();
		SvcRunning = TRUE;
        break;
 
    default:
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    return TRUE;
}


//----------------------------------------------------------------------
//
// AMDSDispatch
//
// In this routine we handle requests to our own device. The only 
// requests we care about handling explicitely are IOCTL commands that
// we will get from the GUI. We also expect to get Create and Close 
// commands when the GUI opens and closes communications with us.
//
//----------------------------------------------------------------------
NTSTATUS AMDSDispatch( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
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
	char msg[]="Hello from log\n";

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
        SvcRunning = TRUE;
        break;

    case IRP_MJ_SHUTDOWN:
        
        //
        // Dump all accumulated buffers. We are in the system process so
        // there's no need to queue a worker thread item
        //
        while( old = AMDSOldestStore()) {
            if( old == Store ) break;
        }
        break;

    case IRP_MJ_CLOSE:

        SvcRunning = FALSE;
        AMDSResetStore();
        break;

    case IRP_MJ_DEVICE_CONTROL:
       	//
        // See if the output buffer is really a user buffer that we
        // can just dump data into.
        //
        if( IOCTL_TRANSFER_TYPE(ioControlCode) == METHOD_NEITHER ) {

            outputBuffer = Irp->UserBuffer;
        }

        //
        // Its a request from the GUI
        //
        AMDSDeviceControl( irpStack->FileObject, TRUE,
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
// AMDSUnload
//
// Our job is done - time to leave.
//
//----------------------------------------------------------------------
VOID AMDSUnload( IN PDRIVER_OBJECT DriverObject )
{
    WCHAR                   deviceLinkBuffer[]  = L"\\DosDevices\\AMDDRV";
    UNICODE_STRING          deviceLinkUnicodeString;


    //
    // Unhook the registry
    //
    if( RegHooked ) UnhookRegistry();
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
    AMDSHashCleanup();
    AMDSFreeStore();

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
    WCHAR                   deviceNameBuffer[]  = L"\\Device\\AMDDRV";
    UNICODE_STRING          deviceNameUnicodeString;
    WCHAR                   deviceLinkBuffer[]  = L"\\DosDevices\\AMDDRV";
    UNICODE_STRING          deviceLinkUnicodeString;    
    WCHAR                   startValueBuffer[] = L"Start";
    UNICODE_STRING          startValueUnicodeString;
    WCHAR                   bootMessage[] = 
        L"\nAMDS is logging Registry activity to \\SystemRoot\\AMDRDRV.LOG\n\n";
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
    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer );
    RtlInitUnicodeString (&deviceLinkUnicodeString,
                          deviceLinkBuffer );

    //
    // Set up the device used for GUI communications
    //
    ntStatus = IoCreateDevice ( DriverObject,
                                0,
                                &deviceNameUnicodeString,
                                FILE_DEVICE_AMDDRV,
                                0,
                                TRUE,
                                &AMDRDev );
    if (NT_SUCCESS(ntStatus)) {
                
        //
        // Create a symbolic link that the GUI can specify to gain access
        // to this driver/device
        //
        ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                                         &deviceNameUnicodeString );

        //
        // Create dispatch points for all routines that must be handled
        //
        DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]        =
        DriverObject->MajorFunction[IRP_MJ_CREATE]          =
        DriverObject->MajorFunction[IRP_MJ_CLOSE]           =
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = AMDSDispatch;
#if DBG

        DriverObject->DriverUnload                          = AMDSUnload;
#endif
    }
    if (!NT_SUCCESS(ntStatus)) {

        //
        // Something went wrong, so clean up (free resources etc)
        //
        if( AMDRDev ) IoDeleteDevice( AMDRDev );
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
    // Initialize rootkey lengths
    //
    for( i = 0; i < NUMROOTKEYS; i++ )
        RootKey[i].RootNameLen = strlen( RootKey[i].RootName );
    for( i = 0; i < 2; i++ )
        CurrentUser[i].RootNameLen = strlen( CurrentUser[i].RootName );

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

        IoDeleteDevice( AMDRDev );
        IoDeleteSymbolicLink( &deviceLinkUnicodeString );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Store->Len  = 0;
    Store->Next = NULL;
    NumStore = 1;

    return STATUS_SUCCESS;
}
//
// Open process token for me please
//
HANDLE AMDOpenEffectiveToken(VOID)
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
    status = ObOpenObjectByPointer( token, 
                                    0, 
                                    NULL,
                                    TOKEN_QUERY, 
                                    NULL, 
                                    KernelMode, 
                                    &tokenHandle 
                                    );

    ObDereferenceObject( token );
    if( !NT_SUCCESS( status )) {

        //
        // We coudn't open the token!!
        //
        return NULL;
    }    
    return tokenHandle;
}

//
// System account 
//

BOOLEAN AMDGetLogonId(HANDLE TokenHandle, char *UserName)
{
    NTSTATUS     status,rtlstat;
    ULONG        requiredLength;
    PSecurityUserData userInformation = NULL;
	int x;
	unsigned char *c,*p;
	ULONG sidlen=0;
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
	sidlen=RtlLengthSid(c);

	for(x=0;x<(int)sidlen;x++) {
		sprintf(p,"%.2X",*(c));
		c++; p=p+2;
		}


    return STATUS_SUCCESS;

}


