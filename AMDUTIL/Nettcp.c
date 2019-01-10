#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include "w32trace.h"
#include "crc.h"
#include "nettcp.h"
//
// Get storage for Info block
//
int InitIB(PINFBLK ib)
{
	if(!ib) return 0;									// Oh Nooooo this is bad...
	memset(ib,0,sizeof(struct InfoBlk));				// clear it out
	ib->srbuf=malloc(SR_BUFF_SIZE);						// Allocate input buffer
	if(!ib->srbuf) {FreeIB(ib); return 0;}				// Noooo free all and go.
	ib->recbuf=malloc(REC_BUFF_SIZE);					// get a recrod buffer
	if(!ib->recbuf) {FreeIB(ib); return 0;}				// Failure get out
return 1;
}
void FreeIB(PINFBLK ib)
{
	if(ib)
		{
		if(ib->srbuf) free(ib->srbuf);					// Line buffer
		if(ib->recbuf) free(ib->recbuf);				// Record Buffer
		free(ib);										// Client Block				
		ib=NULL;										// reset block
		}
return;
}
//
// Recv the next transaction
//
int	RecvTDH(PINFBLK ib)
{
PTDH t;													// Pointer to TDH
char *c;												// pointer to buffer

	if(!ib->srbuf_remain) {if(!RecvBDH(ib)) return 0;}	// Set the transaction type 
	(char*)t=ib->srbuf+ib->srbuf_read;					// set adress of next trans
	c=(char *)t+sizeof(struct tdh);						// set to data
	ib->tdhtype=t->tdhtype;								// Set the type
	ib->tdhdesc=t->tdhdesc;								// Set the description
	ib->tdhfunc=t->tdhfunc;								// Set function
	ib->tdhres=t->tdhres;								// set the reserved

	memset(ib->workbuf,0,TDHLEN+1);						// Clear out buffer
	memcpy(ib->workbuf,t->tdhlen,TDHLEN);				// Copy to null term buffer
	ib->recbuf_len=atoi(ib->workbuf);					// Get length of this record
	memcpy(ib->recbuf,c,ib->recbuf_len);				// Copy in the data
	ib->srbuf_read=ib->srbuf_read+ib->recbuf_len+sizeof(struct tdh);// Increment # of bytes read
	ib->srbuf_remain=ib->srbuf_remain-(ib->recbuf_len+sizeof(struct tdh));
	lmsg(MSG_DEBUG,"Read TDH Complete remain[%d],RecLen[%d]",(void *)ib->srbuf_remain,(void *)ib->recbuf_len);

return 1;
}
//
// Get next block in
//
int	RecvTcp(PINFBLK ib)
{
char *c;
int len=0;														// Length of read
int remain=0;													// Buffer left
PBDH b;															// Define block header
int blen=0;														// Length for this block

	c=ib->srbuf;												// Set to start of buffer
	len=recv(ib->sock,c,sizeof(struct bdh),0);					// Get BDH off input queue
	if(len<0) return 0;											// Session may be gone
	ib->srbuf_ilen=len;											// reset ilen						
	(char *)b=ib->srbuf;										// Point to it
	memset(ib->workbuf,0,BDHLEN+1);								// Clear out buffer
	memcpy(ib->workbuf,b->bdhlen,BDHLEN);						// Copy to null term buffer
	blen=atoi(ib->workbuf);										// Get Length of this message
	blen-=len;													// Next portion please
	while( (blen) && (len!=0) )									// See if data present
		{
		lmsg(MSG_DEBUG,"Recv loop length[%d]",len);				// Send out the message
		c=ib->srbuf+ib->srbuf_ilen;								// poition at next
		len=recv(ib->sock,c,blen,0);							// Get off input queue
		blen-=len;												// Next portion please
		ib->srbuf_ilen+=len;									// Update length
		}
return ib->srbuf_ilen;				
}

//
// Get next block in
//
int	RecvBDH(PINFBLK ib)
{
PBDH b;													// Define block header

	memset(ib->srbuf,0,SR_BUFF_SIZE);					// Clear ou the bufer
	RecvTcp(ib);
	lmsg(MSG_INFO,"Recv complete with total length[%d]",(void *)ib->srbuf_ilen); 
	// DumpArea(ib->srbuf,ib->srbuf_ilen);
	if(ib->srbuf_ilen==0 ||								// Check for errors 
	   ib->srbuf_ilen==SOCKET_ERROR) {ib->error=BADBLK; return 0;}
	(char *)b=ib->srbuf;										// Point to it
	if(b->bdhcrc[0]>' ')								// If CRC provided
	  {
	  GenCrc((char *)b+CRCLEN,ib->srbuf_ilen-CRCLEN,ib->bdhcrc);// See if data is the same
	  if(strnicmp(ib->bdhcrc,b->bdhcrc,8)!=0) 
		{
		ib->error=BADBDH; 
		lmsg(MSG_ERR,"Invalid CRC for block I:[%.8s] O:[%.8s]",(void *)b->bdhcrc,(void *)ib->bdhcrc);
		return 0;
		} 
	  }
	ib->srbuf_remain=ib->srbuf_ilen-sizeof(struct bdh);	// Data that remains
	ib->srbuf_read=sizeof(struct bdh);					// How Many bytes read in

	memset(ib->workbuf,0,MSGLEN+1);						// Clear out buffer
	memcpy(ib->workbuf,b->bdhmsg,MSGLEN);				// Copy to null term buffer
	ib->msgnum=atoi(ib->workbuf);						// Set mesage number
	ib->srbuf_slen=0;									// reset send length
	lmsg(MSG_DEBUG,"Block num[%d] Len[%d] CRC[%.8s] read complete",(void *)ib->msgnum,(void *)ib->srbuf_ilen,ib->bdhcrc);
	return 1;
}
//
// Send data on the line
//
int	SendTrans(PINFBLK ib)
{
PBDH b;	
int rc;
														// Define block header
	if(ib->srbuf_slen<1) 
		{
		lmsg(MSG_ERR,"Send Transaction called with no data in buffer len[%d]",(void *)ib->srbuf_slen);
		return 1;												// No data to send
		}
	(char *)b=ib->srbuf;										// Point to it
	sprintf(b->bdhlen,"%.4d",ib->srbuf_slen);					// Put in the length 
    GenCrc((char *)b+CRCLEN,ib->srbuf_slen-CRCLEN,b->bdhcrc);	// Punt in the CRC Value
	rc=1;
	rc=setsockopt(ib->sock,IPPROTO_TCP,TCP_NODELAY,(const char *)&rc,sizeof(int));
	rc=send(ib->sock,ib->srbuf,ib->srbuf_slen,0);
	if(rc<=0) {
		ib->error=WSAGetLastError();
		lmsg(MSG_INFO,"Send completed with RC[%d] Length[%d]",(void *)ib->error,(void*)ib->srbuf_slen);
		sprintf(ib->recbuf,"%d",ib->error);
		return 0;
		}
	ib->srbuf_slen=0;											// Reset send length
	ib->srbuf_remain=0;											// Reset the length
	return 1;
}
//
// Start Trans always starts the buffer so we know where the tdh lives and leave
// room for the bdh
//
int StartTrans(PINFBLK ib,char type,char func,int data)
{
PTDH t;
char *c;

	memset(ib->srbuf,0,SR_BUFF_SIZE);						// Clear out send buffer
	ib->srbuf_slen=sizeof(struct bdh)+sizeof(struct tdh);	// Clear out send size
	(char *)t=(ib->srbuf+sizeof(struct bdh));				// Set tdh position
	c=(char *)t+sizeof(struct tdh);							// Set where data goes
	t->tdhtype=type;										// Set the type
	ib->tdhtype=type;										// Save in IB
	t->tdhdesc=STARTSECTION;								// Set the description
	t->tdhfunc=func;										// Set the function
	ib->tdhfunc=func;										// Save in IB
	if(data) 
		{
		sprintf(t->tdhlen,"%.4d",ib->recbuf_len);			// Set the data length with the tdh
		memcpy(c,ib->recbuf,ib->recbuf_len);		 		// move data into bufer
		ib->srbuf_slen+=ib->recbuf_len;						// Next position in buffer
		}
	if(!SendTrans(ib)) return 0;
return 1;
}
//
// Start Trans always starts the buffer so we know where the tdh lives and leave
// room for the bdh
//
int InitTrans(PINFBLK ib)
{

	memset(ib->srbuf,0,SR_BUFF_SIZE);						// Clear out send buffer
	ib->srbuf_slen=sizeof(struct bdh);						// Clear out send size
return 1;
}

//
// Send data for Started transaction
//
int SendDataOnTrans(PINFBLK ib)
{
PTDH t;
char *c;
			
	if(ib->tdhtype==UNKN)
		{
		WriteLog("ERROR: SendDataOnTrans Called Without StartTran");
		return 0;
		}
    if( (ib->recbuf_len+ib->srbuf_slen+sizeof(struct tdh)) > SR_BUFF_SIZE) // No More room
		{
		SendTrans(ib);											// Send out the data
		memset(ib->srbuf,0,SR_BUFF_SIZE);						// Clear out send buffer
		ib->srbuf_slen=sizeof(struct bdh)+sizeof(struct tdh);	// Clear out send size
		(char *)t=(ib->srbuf+sizeof(struct bdh));				// Set tdh position
		}
	else (char *)t=(ib->srbuf+ib->srbuf_slen);					// Set tdh position
	c=(char *)t+sizeof(struct tdh);								// Set where data goes
	t->tdhtype=ib->tdhtype;										// Set the type
	t->tdhdesc=DATAONLY;										// Set the description
	t->tdhfunc=ib->tdhfunc;										// Set the function
	sprintf(t->tdhlen,"%.4d",ib->recbuf_len);					// Set the data length with the tdh
	memcpy(c,ib->recbuf,ib->recbuf_len);		 				// move data into bufer
	ib->srbuf_slen+=ib->recbuf_len+sizeof(struct tdh);			// Next position in buffer
return 1;
}
//
// Signal No more data for transaction
//
int StopDataOnTrans(PINFBLK ib)
{
PTDH t;

    if( (ib->recbuf_len+sizeof(struct tdh)) > SR_BUFF_SIZE)		// No More room
		{
		SendTrans(ib);											// Send out the data
		memset(ib->srbuf,0,SR_BUFF_SIZE);						// Clear out send buffer
		ib->srbuf_slen=sizeof(struct bdh)+sizeof(struct tdh);	// Clear out send size
		(char *)t=(ib->srbuf+sizeof(struct bdh));				// Set tdh position
		}
	else (char *)t=(ib->srbuf+ib->srbuf_slen);					// Set tdh position
	t->tdhtype=ib->tdhtype;										// Set the type
	t->tdhdesc=ENDSECTION;										// Set the description
	t->tdhfunc=ib->tdhfunc;										// Set the function
	sprintf(t->tdhlen,"%.4s","0000");							// Set to 0
	ib->srbuf_slen+=sizeof(struct tdh);							// Next position in buffer
	ib->tdhtype=UNKN;											// Reset
	ib->tdhdesc=UNKN;											// the
	ib->tdhfunc=UNKN;											// Transaction
	
return 1;
}
