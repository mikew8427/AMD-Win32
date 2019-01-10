#define SR_BUFF_SIZE 16384
#define REC_BUFF_SIZE 8192
#define TDHLEN 4
#define BDHLEN 6
#define MSGLEN 4
/***********************************************
Define the error code here
************************************************/
#define BADBLK			500
#define BADBDH			505
#define BADTDH			510
#define BADCRC			515
/***********************************************
Define Transaction Codes Here
************************************************/
#define UNKN			0x00			// Unknown transaction
#define INIT			0x01			// Initaillize tranaction
#define EOT				0x02			// End of transaction
#define SHUT			0x03			// Shutdown
#define RSPNS			0x04			// Response
/***********************************************
Define Description Codes Here
************************************************/
#define STARTSECTION	0x01			// Start of a data section
#define MIDSECTION		0x02			// Middle of a data section
#define ENDSECTION		0x03			// End of a data sections
#define DATAONLY		0x04			// Data only flow with trans 

/************************************************
Transaction Definition Header
*************************************************/
typedef struct tdh           
{
unsigned char	tdhtype;					// Type for this transaction
unsigned char	tdhdesc;					// Description for the TYPE
unsigned char	tdhfunc;					// Fuction if needed
unsigned char	tdhres;						// Reserved
char			tdhlen[4];				    // Length of data in char form
} TDH,*PTDH;

/************************************************
Block Definition Header
*************************************************/
typedef struct bdh           
{
char			bdhcrc[8];					// CRC of this block
char			bdhmsg[4];					// Message Number
char			bdhlen[6];				    // Length of data in char form
char			bdhres[6];					// Reserved
} BDH,*PBDH;
/************************************************
Information block used in Client Conversation
*************************************************/
typedef struct InfoBlk           
{
char			*srbuf;					    // @ of send/recv buffer
char			*recbuf;					// Current record from srbuf
char			bdhcrc[8];					// BDH CRC value
int				msgnum;						// Current message number
unsigned char	tdhtype;					// Current tdh type
unsigned char	tdhdesc;					// Current tdh description
unsigned char   tdhfunc;					// Current tdh function
unsigned char   tdhres;						// Reserved
int				recbuf_len;					// Number of bytes in recbuf
int				srbuf_remain;				// Number of bytes left in srbuf
int				srbuf_read;					// Number of bytes read from srbuf
int				srbuf_ilen;					// Number of bytes read off line
int				srbuf_slen;					// Number of bytes to send on line
int				term_session;				// Terminate Session Indicator
int				error;						// Error number
SOCKET			sock;						// socket number 
char			workbuf[1024];				// Temp work buffer
} INFBLK,*PINFBLK;
int		InitIB(PINFBLK ib);				    // Init routine for Info block  
void	FreeIB(PINFBLK ib);					// Free Info Block
int		InitTrans(PINFBLK ib);				// Init the buffers in the IB
void	StopClient(SOCKET s);				// Shutdown the Client thread 
int		RecvTDH(PINFBLK ib);				// Recv of Next transaction
int		RecvBDH(PINFBLK ib);				// Recv of New block off line
int		StartTrans(PINFBLK ib,char type,char func,int data);
int		SendTrans(PINFBLK ib);
int		SendDataOnTrans(PINFBLK ib);		// Data from Transaction
int     StopDataOnTrans(PINFBLK ib);		// End of data for transaction
