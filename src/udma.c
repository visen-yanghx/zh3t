/*
 * udma.c
 *
 *  Created on: May 22, 2018
 *      Author: terence
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <time.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>


#include "udma.h"
#include "ugpio.h"
#include "ummap.h"


//#define IOCTL_DMA_RD		(_IOC(_IOC_READ, 'D', 1, 4))
//#define IOCTL_DMA_WR		(_IOC(_IOC_WRITE, 'D', 0, 4))


#define IOCTL_MAGIC					'D'
#define NUM_IOCTLS					3
#define IOCTL_DMA_RD				(_IOR(IOCTL_MAGIC, 0, unsigned long))
#define IOCTL_DMA_WR				(_IOR(IOCTL_MAGIC, 1, unsigned long))
#define IOCTL_DMA_SA				(_IOR(IOCTL_MAGIC, 2, unsigned long))
#define IOCTL_DMA_DA				(_IOR(IOCTL_MAGIC, 3, unsigned long))
#define BUFFER_SIZE 				0x200000


#define DMA_TRANS_SIZE 		0x8000				  // DMA transfer size once time
#define TEST_LEN     		384 * 1024
#define TEST_LOOP_COUNT		10000

static int ddr_perfmance_test();

static int dma_do_rx_test(int dma_num, int dma, int file, void* virtaddr, int len);
static int dma_do_tx_test(int dma_num, int dma, int file, void* virtaddr, int len);

static int dma0_test_routine(void);
static int dma1_test_routine(void);
static int dma2_test_routine(void);

double calc_duration(struct timeval start_time, struct timeval end_time);


//const char* axidma_rxfile[] = {"/smbshare/chan0_rx", "/smbshare/chan1_rx", "/smbshare/chan2_rx"};
const char* axidma_rxfile[] = {"/home/netlogon/nfs/chan0_rx", "/home/netlogon/nfs/chan1_rx", "/home/netlogon/nfs/chan2_rx"};
//const char* axidma_rxfile[] = {"/media/p1/chan0_rx", "/media/p2/chan1_rx", "/media/p3/chan2_rx"};
const char *index_file = "/home/netlogon/nfs/chan0_index";

unsigned int axidma_rxmem[] = {0x7F100000, 0x7F300000, 0x7F500000, 0x7F700000};

AxiDma axidma[] = {
		{"axidma0", "/dev/axidma0", 0, 0, 0, 0x7F000000, NULL, 0x100000, 0x7F100000, NULL, 0x100000},
		{"axidma1", "/dev/axidma1", 1, 0, 0, 0x7F200000, NULL, 0x100000, 0x7F300000, NULL, 0x100000},
		{"axidma2", "/dev/axidma2", 2, 0, 0, 0x7F400000, NULL, 0x100000, 0x7F500000, NULL, 0x100000},
		{"axidma3", "/dev/axidma3", 3, 0, 0, 0x7F600000, NULL, 0x100000, 0x7F700000, NULL, 0x100000}
};

void *aligned_buffer;

uint32_t dma_rdable_count[3] = {0};
uint32_t dma_rd_count[3] = {0};
uint32_t dma_writable_count[3] = {0};
uint32_t dma_written_count[3] = {0};

unsigned char gMutexDma = 0;// mutex for dma.(FOR DEBUG)

int dma_init()
{
    // open DMA
    int i = 0;
    unsigned int page_frame_number = 0;

    for (i = 0; i < 4; i++) {
    	axidma[i].dma_num = i;
    	axidma[i].fd = dma_open(axidma[i].path);
    	if(axidma[i].fd < 0){
    		printf("Failed to Open %s.\n", axidma[i].name);
    		return -1;
    	}

//    	axidma[i].rxmem_vbase = mmap_create(axidma[i].rxmem_pbase, axidma[i].rxmem_size);
//    	if (axidma[i].rxmem_vbase == MAP_FAILED) {
//    		printf("Failed to map memory for %s.\n", axidma[i].name);
//    		return -1;
//    	}
//
//    	axidma[i].txmem_vbase = mmap_create(axidma[i].txmem_pbase, axidma[i].txmem_size);
//    	if (axidma[i].txmem_vbase == MAP_FAILED) {
//    		printf("Failed to map memory for %s.\n", axidma[i].name);
//    		return -1;
//    	}
//
//    	axidma[i].mapped = 1;

    	// TX
		// Create a buffer with some data in it
    	axidma[i].txmem_vbase = create_buffer(BUFFER_SIZE);
		// Get the page frame the buffer is on
		page_frame_number = get_page_frame_number_of_address(axidma[i].txmem_vbase);
		//printf("Page frame: 0x%x\n", page_frame_number);

		// Find the difference from the buffer to the page boundary
		//distance_from_page_boundary = (unsigned long)axidma[i].txmem_vbase % getpagesize();

		// Determine how far to seek into memory to find the buffer
		//offset = (page_frame_number << PAGE_SHIFT) + distance_from_page_boundary;

		ioctl(axidma[i].fd, IOCTL_DMA_SA, &page_frame_number);

		// RX
		// Create a buffer with some data in it
    	axidma[i].rxmem_vbase = create_buffer(BUFFER_SIZE);

		// Get the page frame the buffer is on
		page_frame_number = get_page_frame_number_of_address(axidma[i].rxmem_vbase);
		//printf("Page frame: 0x%x\n", page_frame_number);

		// Find the difference from the buffer to the page boundary
		//distance_from_page_boundary = (unsigned long)axidma[i].rxmem_vbase % getpagesize();

		// Determine how far to seek into memory to find the buffer
		//offset = (page_frame_number << PAGE_SHIFT) + distance_from_page_boundary;

		ioctl(axidma[i].fd, IOCTL_DMA_DA, &page_frame_number);

    }

//	posix_memalign(&aligned_buffer, 512, TEST_LEN);
//	memset(aligned_buffer, 0x5A, TEST_LEN);

    usleep(50);

    return 0;
}

void dma_release()
{
    int i = 0;
    for (i = 0; i < 4; i++) {
    	close(axidma[i].fd);
    	munmap(axidma[i].rxmem_vbase, axidma[i].rxmem_size);
    	axidma[i].mapped = 0;
    }

    free(aligned_buffer);
}

static void dma_reserved_memory_test()
{
	int count = 0, loop = 0;
	unsigned int buffer[0x2000];
	for (count = 0; count < 0x2000; count++)
	{
		buffer[count] = count + 1;
	}

	for (loop = 0; loop < 100; loop++)
	{
		memcpy(axidma[1].rxmem_vbase + loop * 0x100000, buffer, 0x8000);
	}

}

/**********************************
 * @return,  return 0 if test successfully. Else return -1.
 */
int dma_selftest()
{
	pthread_t dma0_test_tid, dma1_test_tid, dma2_test_tid;

//	dma_reserved_memory_test();
//	ddr_perfmance_test();
//	/pthread_create(&dma0_test_tid, NULL, (void*)dma0_test_routine, NULL);
//	pthread_create(&dma1_test_tid, NULL, (void*)dma1_test_routine, NULL);
//	pthread_create(&dma2_test_tid, NULL, (void*)dma2_test_routine, NULL);

	return 0;

}

static int ddr_perfmance_test()
{
	int ret = 0;
//	int i = 0;
	int loop = 0;
	int test_loop = TEST_LOOP_COUNT;
	double duration = 0, speed = 0;
	struct timeval start_time, end_time;
	uint32_t  len = TEST_LEN;

	unsigned char buffer[TEST_LEN] = {0};
	//unsigned char buffer1[TEST_LEN] = {0};

	//Read back the data form AXI-DMA Device in the userspace receive buffer
	gettimeofday(&start_time, NULL);
	while (loop < test_loop) {
		loop++;

		//memcpy(aligned_buffer, buffer, len);
		//memcpy(buffer, aligned_buffer, len);
		//memcpy(aligned_buffer, virtaddr, len);
		//memcpy(virtaddr, aligned_buffer, len);
		//memcpy(buffer, virtaddr, len);
		//memcpy(virtaddr, buffer, len);
		//memcpy(buffer, buffer1, len);

		//ret = dma_fwrite(dma_num, dma, file, virtaddr, len);
		//lseek(file, 0, SEEK_SET);
		//printf("data read 0x%x\n", ret);
	}
	gettimeofday(&end_time, NULL);
	calc_duration(start_time, end_time);
	duration = calc_duration(start_time, end_time);
	speed = (double)((uint64_t)len  * test_loop / 1024 /1024) / duration;		// MB/s
	printf("Transfer %u (MB), elapse time: %f (s), speed: %f (MB/s)\n",
			(uint32_t)((uint64_t)len * test_loop / 1024 /1024), duration, speed);
	return 0;
}


static int dma_do_rx_test(int dma_num, int dma, int file, void* virtaddr, int len)
{
	int ret = 0;
	int loop = 0;
	int test_loop = TEST_LOOP_COUNT;
	double duration = 0, speed = 0;
	struct timeval start_time, end_time;

	//Read back the data form AXI-DMA Device in the userspace receive buffer
	gettimeofday(&start_time, NULL);
	while (loop < test_loop) {
		loop++;
		ret = dma_fwrite(dma_num, dma, file, virtaddr, len);
	}
	gettimeofday(&end_time, NULL);
	//calc_duration(start_time, end_time);
	duration = calc_duration(start_time, end_time);
	speed = (double)((uint64_t)len  * test_loop / 1024 /1024) / duration;		// MB/s
	printf("DMA read %u (MB), elapse time: %f (s), speed: %f (MB/s)\n",
			(uint32_t)((uint64_t)len  * test_loop / 1024 /1024), duration, speed);
	return 0;
}


static int dma_do_tx_test(int dma_num, int dma, int file, void* virtaddr, int len)
{
	int ret = 0;
	int loop = 0;
	int test_loop = TEST_LOOP_COUNT;
	double duration = 0, speed = 0;
	struct timeval start_time, end_time;

	//Read back the data form AXI-DMA Device in the userspace receive buffer
	gettimeofday(&start_time, NULL);
	while (loop < test_loop) {
		loop++;
		read(file, virtaddr, len);
		ret = dma_ioctl_read(dma_num, dma, 0, len);
	}
	gettimeofday(&end_time, NULL);
	calc_duration(start_time, end_time);
	duration = calc_duration(start_time, end_time);
	speed = (double)((uint64_t)len  * test_loop / 1024 /1024) / duration;		// MB/s
	printf("DMA read %u (MB), elapse time: %f (s), speed: %f (MB/s)\n",
			(uint32_t)((uint64_t)len  * test_loop / 1024 /1024), duration, speed);
	return 0;
}


static int dma0_test_routine()
{
	int fd;
//	pthread_detach(pthread_self());

	//fd = open(axidma_rxfile[0], O_CREAT | O_TRUNC | O_RDWR /*|O_SYNC*/, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
//	fd = open(axidma_rxfile[0], FLAGS, MODE);
//	if(fd < 0){
//		printf("%s() Failed to Open file.\n", __func__);
//		return -1;
//	}

	//mmap(axidma[0].rxmem_vbase, TEST_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	dma_do_rx_test(axidma[0].dma_num, axidma[0].fd, fd, axidma[0].rxmem_vbase, TEST_LEN);

	close(fd);
	return 0;
}

static int dma1_test_routine()
{
	int fd;

	//fd = open(axidma_rxfile[1], O_CREAT | O_TRUNC | O_RDWR |O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	fd = open(axidma_rxfile[1], FLAGS, MODE);
	if(fd < 0){
		printf("%s() Failed to Open file.\n", __func__);
		return -1;
	}

	dma_do_rx_test(axidma[1].dma_num, axidma[1].fd, fd, axidma[1].rxmem_vbase, 0x1000000/*TEST_LEN*/);

	close(fd);
	return 0;
}

static int dma2_test_routine()
{
	int fd;

//	pthread_detach(pthread_self());

	//fd = open(axidma_rxfile[2], O_CREAT | O_TRUNC | O_RDWR |O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	fd = open(axidma_rxfile[2], FLAGS, MODE);
	if(fd < 0){
		printf("%s() Failed to Open file.\n", __func__);
		return -1;
	}

	dma_do_rx_test(axidma[2].dma_num, axidma[2].fd, fd, axidma[2].rxmem_vbase, TEST_LEN);

	close(fd);
	return 0;
}


/****************************
 * @dmaname,  dma device name
 * @return,   device handle if open dma device successfully.
 * 			  Otherwise, return a negative value.
 */
int dma_open(const char* dmaname)
{
	int dh = open(dmaname, O_RDONLY | O_SYNC);
	if(dh < 0){
		printf("Failed to Open device.\n");
	}

	return dh;
}

/**************************************
 * @fd, 	  dma device handle
 * @buffer,   buffer to hold data written to device
 * @len,      length of data want to write. len should be times of DMA_TRANS_SIZE.
 * @return,	  bytes written.
 */
int dma_write(int dma_num, int fd, void *buffer, int len)
{
	int bytes_written = 0;
	int bytes = 0;

	//write the user-space buffer to AXI-DMA Device
	for (; bytes < len; bytes += DMA_TRANS_SIZE) {
		if (dma_buf_writable(dma_num))
		{
			dma_writable_count[dma_num]++;
			if (DMA_TRANS_SIZE == write(fd, buffer + bytes, DMA_TRANS_SIZE)) {
				dma_written_count[dma_num]++;
			}
		}
	}

	return bytes_written;
}


/**********************************
 * @fd, 	  dma device handle
 * @buffer,   buffer to hold data read from device
 * @len,      length of data want to read
 * @return,	  bytes read
 */
int dma_read(int dma_num, int fd, void* buffer, int len)
{
	int bytes_read = 0;
	int bytes = 0;

	//Read data form AXI-DMA Device into the user-space receive buffer
	for (bytes = 0; bytes < len; bytes += DMA_TRANS_SIZE) {
		if (dma_buf_readable(dma_num)) {
			if (DMA_TRANS_SIZE == read(fd, buffer + bytes, DMA_TRANS_SIZE)) {
				bytes_read += DMA_TRANS_SIZE;
				dma_buf_clear(dma_num);
			}
		}
	}

	return bytes_read;
}


/**************************************
 * @fd, 	  dma device handle
 * @len,      length of data want to write. len should be times of DMA_TRANS_SIZE.
 * @pos,      specify the offset to the dma receive memory base.
 * @return,	  bytes written.
 * @note      ioctl() write bytes equals DMA_TRANS_SIZE
 */
int dma_ioctl_write(int dma_num, int fd, long pos, int len)
{
	int bytes_written = 0;
	int bytes = 0;
	unsigned long offset = 0;

	//write the user-space buffer to AXI-DMA Device
	for (; bytes < len; bytes += DMA_TRANS_SIZE) {
		if (dma_buf_writable(dma_num))
		{
			offset = pos + bytes;
			dma_writable_count[dma_num]++;
			//printf("@\n");
			if (ioctl(fd, IOCTL_DMA_WR, &offset) == DMA_TRANS_SIZE) {
				dma_written_count[dma_num]++;
				bytes_written += DMA_TRANS_SIZE;
			}
		}
	}

	return bytes_written;
}


/**********************************
 * @fd, 	  dma device handle
 * @len,      length of data want to read
 * @pos,      specify the offset to the dma receive memory base.
 * @return,	  bytes read
 * @note      ioctl() read bytes equals DMA_TRANS_SIZE
 */
int dma_ioctl_read(int dma_num, int fd, long pos, int len)
{
	int bytes_read = 0;
	int bytes = 0;
	int i = 0;
	unsigned long offset = 0;

	//printf("@%d\n", dma_num);
	//Read data form AXI-DMA Device into the user-space receive buffer
	for (bytes = 0; bytes < len; bytes += DMA_TRANS_SIZE) {
		//if (dma_buf_readable(dma_num))
	//	{
			offset = pos + bytes;
			dma_rdable_count[dma_num]++;
			while(gMutexDma > 0)
			{
				for(i=0;i<1000;i++)
				{
					;
				}
			}
			gMutexDma = 1;
			if (ioctl(fd, IOCTL_DMA_RD, &offset) == DMA_TRANS_SIZE) {
				dma_rd_count[dma_num]++;
				bytes_read += DMA_TRANS_SIZE;
			}
			gMutexDma = 0;
			//dma_buf_clear(dma_num);
			//printf("dma_buf_clear(dma_num):read");

		}

	//printf("@dma %d readable/read: %d/%d\n", dma_num, dma_rdable_count[dma_num], dma_rd_count[dma_num]);
	return bytes_read;
}


/**********************************
 * @dma 	  dma device handle
 * @file      file description
 * @virtaddr  buffer to store data read. len should be times of DMA_TRANS_SIZE
 * @len,      length of data want to read
 * @return,	  bytes written
 */
int dma_fwrite(int dma_num, int dh, int fd, void *virtaddr, int len)
{
	int bytes_read = 0;
	int bytes_written = 0;
	int bytes = 0;


//	//Read data form AXI-DMA Device into the user-space receive buffer
//	for (bytes = 0; bytes < len; bytes += DMA_TRANS_SIZE) {
//		bytes_read += dma_read(dh, DMA_TRANS_SIZE);
//		bytes_written += write(fd, virtaddr, DMA_TRANS_SIZE);
//	}

	//	printf("data read 0x%x\n", bytes_read);
	//	if (bytes_read < len) {
	//     	 printf("Failed to read from the device\n");
	//    }

	bytes_read = dma_ioctl_read(dma_num, dh, 0, len);
	//bytes_read = dma_read(dh, virtaddr, len);
	//memcpy(aligned_buffer, virtaddr, len);
	//bytes_written = write(fd, virtaddr, bytes_read);

	// TEST
	//write(fd, virtaddr, len);

	return bytes_written;
}


double calc_duration(struct timeval start_time, struct timeval end_time)
{
    struct timeval diff_time;
    double duration = 0.0;
    if (end_time.tv_usec < start_time.tv_usec) {
        diff_time.tv_sec  = end_time.tv_sec  - start_time.tv_sec  - 1;
        diff_time.tv_usec = end_time.tv_usec - start_time.tv_usec + 1000*1000;
    } else {
        diff_time.tv_sec  = end_time.tv_sec  - start_time.tv_sec ;
        diff_time.tv_usec = end_time.tv_usec - start_time.tv_usec;
    }

    duration = diff_time.tv_sec +  (double)diff_time.tv_usec / 1000 / 1000;
    //printf("time = %ld.%06ld sec\n", diff_time.tv_sec, diff_time.tv_usec);
    return duration;
}
