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
//                          dec 54,781
#define FILE_DEVICE_AMDDRV      0x0000d5fd

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



