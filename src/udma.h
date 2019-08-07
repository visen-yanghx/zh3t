/*
 * libdma.h
 *
 *  Created on: May 22, 2018
 *      Author: terence
 */

#ifndef SRC_UDMA_H_
#define SRC_UDMA_H_


//定义flags:只写，文件不存在那么就创建，文件长度戳为0
#define FLAGS 		O_RDWR | O_CREAT  | O_TRUNC | __O_DIRECT
//#define FLAGS 		O_RDWR | O_CREAT  | O_TRUNC
//创建文件的权限，用户读、写、执行、组读、执行、其他用户读、执行
#define MODE 		S_IRWXU | S_IXGRP | S_IROTH | S_IXOTH


#define DMA_RXMEM_SIZE			0x100000
#define DMA_BUF_SIZE			384 * 1024

typedef struct _axidma {
	const char name[8];
	const char path[16];
	int dma_num;
	int fd;
	uint8_t mapped;
	uint32_t txmem_pbase;
	uint8_t *txmem_vbase;
	uint32_t txmem_size;
	uint32_t rxmem_pbase;
	uint8_t *rxmem_vbase;
	uint32_t rxmem_size;
} AxiDma;


extern const char* axidma_rxfile[];
extern unsigned int axidma_rxmem[];
extern const char *index_file;
extern AxiDma axidma[];


int dma_init();
void dma_release();
int dma_selftest();
int dma_open(const char* dmaname);
void select_dma_mode(uint8_t mode);

int dma_write(int dma_num, int fd, void *buffer, int len);
int dma_read(int dma_num, int fd, void *buffer, int len);

int dma_ioctl_write(int dma_num, int fd, long pos, int len);
int dma_ioctl_read(int dma_num, int fd, long pos, int len);

int dma_fwrite(int dma_num, int dma_fd, int file_fd, void *virtaddr, int len);

double calc_duration(struct timeval start_time, struct timeval end_time);

#endif /* SRC_UDMA_H_ */
