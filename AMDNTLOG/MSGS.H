//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: AMD_INFO
//
// MessageText:
//
//  AMD Process [%1] has issued Information Message [%2]
//
#define AMD_INFO                         0x40000001L

//
// MessageId: AMD_WARN
//
// MessageText:
//
//  AMD Process [%1] has issued Warning Code [%2]
//
#define AMD_WARN                         0x80000002L

//
// MessageId: AMD_ERROR
//
// MessageText:
//
//  AMD Process [%1] has issued Error Code [%2]
//
#define AMD_ERROR                        0x80000003L

//
// MessageId: AMD_USER_INFO
//
// MessageText:
//
//  AMD User Information [%1] [%2]
//
#define AMD_USER_INFO                    0x40000004L

//
// MessageId: AMD_USER_WARN
//
// MessageText:
//
//  AMD User Warning [%1] [%2]
//  
//
#define AMD_USER_WARN                    0x80000005L

//
// MessageId: AMD_USER_ERROR
//
// MessageText:
//
//  AMD User Error [%1] [%2]
//
#define AMD_USER_ERROR                   0x80000006L

