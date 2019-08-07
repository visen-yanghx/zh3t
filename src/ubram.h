/*
 * ubram.h
 *
 *  Created on: Jun 7, 2018
 *      Author: terence
 */

#ifndef SRC_UBRAM_H_
#define SRC_UBRAM_H_


#include <linux/ioctl.h>


#define BRAM0_BA			0x80000000
#define BRAM1_BA			0x82000000
#define BRAM_SIZE			0x2000


extern unsigned char* index_aligned_buffer;
extern unsigned long long *bram_vptr[2];

void ubram_init();
void ubram_release();
int ubram_map();
void ubram_unmap();
int ubram_read(int bram_id, unsigned char *buffer, long int size);
int ubram_write(int bram_id, unsigned char *buffer, long int size);
int ubram_fwrite(int bram_id, int fd, long int size);
int ubram_selftest(void);

uint32_t chan_fifo_count(uint8_t chan);
uint8_t index_buf_valid();
void index_buf_invalid();
uint8_t chan_cut_req(uint8_t chan);
void chan_cut_ack(uint8_t chan);

#endif /* SRC_UBRAM_H_ */
