#include "windows.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "crc.h"
#define POLYNOMIAL 0x04c11db7L
static unsigned long crc_table[256];
/*****************************************************************
Build the crc table that will be used by update_crc
*****************************************************************/
void gen_crc_table()
 /* generate the table of CRC remainders for all possible bytes */
 { register int i, j;  register unsigned long crc_accum;
   for ( i = 0;  i < 256;  i++ )
       { crc_accum = ( (unsigned long) i << 24 );
         for ( j = 0;  j < 8;  j++ )
              { if ( crc_accum & 0x80000000L )
                   crc_accum =
                     ( crc_accum << 1 ) ^ POLYNOMIAL;
                else
                   crc_accum =
                     ( crc_accum << 1 ); }
         crc_table[i] = crc_accum; }
   return; }

unsigned long update_crc(unsigned long crc_accum, char *data_blk_ptr,
                                                    int data_blk_size)
 /* update the CRC on the data block one byte at a time */
 { register int i, j;
   for ( j = 0;  j < data_blk_size;  j++ )
       { i = ( (int) ( crc_accum >> 24) ^ *data_blk_ptr++ ) & 0xff;
         crc_accum = ( crc_accum << 8 ) ^ crc_table[i]; }
   return crc_accum; }
/*****************************************************************
Generate and format a char CRC value
*****************************************************************/
int GenCrc(char *data,int len,char *crc)
{
unsigned long crc_accum=0;
unsigned long crcret=0;

	crcret=update_crc(crc_accum,data,len);		
	sprintf(crc,"%.8X",crcret);
	return 1;
}
