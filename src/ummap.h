/*
 * libmmap.h
 *
 *  Created on: Mar 23, 2018
 *      Author: terence
 */

#ifndef SRC_UMMAP_H_
#define SRC_UMMAP_H_


#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <memory.h>

//static int fd;
//#define MODE (O_WRONLY | O_TRUNC)

int mmap_init();
void mmap_exit();
void* mmap_create(uint32_t phyaddr, size_t len);
int mmap_release(void *virtaddr, size_t len);

//void reg_out32(uint64_t phyaddr, uint32_t val);
//int reg_in32(uint64_t phyaddr);
//void reg_write32(unsigned int addrBase, unsigned int addrOffset, unsigned int value);
//int reg_read32(unsigned int addrBase, unsigned int addrOffset);


void* create_buffer(size_t buf_size);
unsigned long get_page_frame_number_of_address(void *addr);


// The page frame shifted left by PAGE_SHIFT will give us the physcial address of the frame
// Note that this number is architecture dependent. For me on x86_64 with 4096 page sizes,
// it is defined as 12. If you're running something different, check the kernel source
// for what it is defined as.
#define PAGE_SHIFT 12
#define PAGEMAP_LENGTH 8


#endif /* SRC_UMMAP_H_ */
