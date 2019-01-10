//======================================================================
// 
// Ioctlcmd.h
//
// Common header file for device driver and Application. Contains common
// data structure definitions and IOCTL command codes.
//
//======================================================================

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//
//                          dec 54,782
#define FILE_DEVICE_AMDDRV      0x0000d5fe

//
// Version #
//
#define AMDDRVVERSION    100


//
// Commands that the GUI can send the device driver
//
#define AMDDRV_hook      (ULONG) CTL_CODE( FILE_DEVICE_AMDDRV, 0x00, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define AMDDRV_unhook    (ULONG) CTL_CODE( FILE_DEVICE_AMDDRV, 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define AMDDRV_zerostats (ULONG) CTL_CODE( FILE_DEVICE_AMDDRV, 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define AMDDRV_getstats  (ULONG) CTL_CODE( FILE_DEVICE_AMDDRV, 0x03, METHOD_NEITHER, FILE_ANY_ACCESS )
#define AMDDRV_setfilter (ULONG) CTL_CODE( FILE_DEVICE_AMDDRV, 0x04, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define AMDDRV_version   (ULONG) CTL_CODE( FILE_DEVICE_AMDDRV, 0x05, METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Format of an output buffer entry
//
typedef struct {
	ULONG	seq;
	char	text[0];
} ENTRY, *PENTRY;

//
// Length of a filter definition string
//
#define MAXFILTERLEN 256
#define MWMAXFILTER	4096
//
// Maximum seperate filter components 
//
#define MAXFILTERS 128
//
// Filter definition
//
typedef struct {
        char     processfilter[MAXFILTERLEN];
		char     processexclude[MAXFILTERLEN];
        char     pathfilter[MWMAXFILTER];
        char     excludefilter[MWMAXFILTER];
        BOOLEAN  logsuccess;
        BOOLEAN  logerror;
        BOOLEAN  logreads;
        BOOLEAN  logwrites;
  		BOOLEAN  mode[MAXFILTERS];
		BOOLEAN	 createmode[MAXFILTERS];
} FILTER, *PFILTER;


//
// Define page size for use by GUI
//
#ifndef PAGE_SIZE
#if defined(_ALPHA_)
#define PAGE_SIZE 0x2000  // 8K
#else
#define PAGE_SIZE 0x1000  // 4K
#endif
#endif

//
// An allocation unit size 
//
#define MAX_STORE	(PAGE_SIZE*8 - 8)

//
// Our different file system types
//
typedef enum {
    GUIINTERFACE,
    STANDARD,
    NPFS,
    MSFS
} FILE_SYSTEM_TYPE, *PFILE_SYSTEM_TYPE;


//
// Size of a storage buffer in bytes - keep it just below 64K so that
// the total length of the buffer including the header is just below
// a page boundary, thus not wasting much space since buffer allocation
// is page-granular.
//
#define LOGBUFSIZE       ((ULONG)(64*0x400-(3*sizeof(ULONG)+1)))

//
// Definition for system call service table
//
typedef struct _SRVTABLE {
	PVOID           *ServiceTable;
	ULONG           LowCall;        
	ULONG           HiCall;
	PVOID		*ArgTable;
} SRVTABLE, *PSRVTABLE;

//
// Pointer to the image of the system service table
//
extern PSRVTABLE KeServiceDescriptorTable;
