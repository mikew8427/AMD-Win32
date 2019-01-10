
/********************************************************************/
/** Include file for CRC.C. Generate a CRC value for a char buffer **/
/********************************************************************/
#define CRCLEN		 8
void gen_crc_table();
unsigned long update_crc(unsigned long crc_accum, char *data_blk_ptr,int data_blk_size);
int GenCrc(char *data,int len,char *crc);