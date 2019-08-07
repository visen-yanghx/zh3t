/*
 * ubram.c
 *
 *  Created on: Jun 8, 2018
 *      Author: terence
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/mman.h>
#include "ubram.h"


#define TEST_LOOP_COUNT		10000
unsigned long long *bram_vptr[2];
unsigned char* index_aligned_buffer = NULL;


void ubram_init()
{
	ubram_map();
	posix_memalign((void**)&index_aligned_buffer, 0x2000, 0x800000);
}

void ubram_release()
{
	ubram_unmap();
	free(index_aligned_buffer);
}

int ubram_map()
{
	int fd = 0;

	if ((fd = open("/dev/mem", O_RDWR /*| O_SYNC*/)) > 0) {
		bram_vptr[0] = (unsigned long long *)mmap(NULL, BRAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BRAM0_BA);//offset =0x80000000
		if (bram_vptr[0] == MAP_FAILED) {
			perror("bram0 map failed\n");
			return -1;
		}

		bram_vptr[1] = (unsigned long long *)mmap(NULL, BRAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BRAM1_BA);//offset  =0x82000000
		if (bram_vptr[1] == MAP_FAILED) {                                                                            //ps to pl 0x82000000
			perror("bram1 map failed\n");                                                                             //pl to ps 0x82001000
			return -1;
		}
	}

	close(fd);

	return 0;
}

void ubram_unmap()
{
	munmap(bram_vptr[0], BRAM_SIZE);
	munmap(bram_vptr[1], BRAM_SIZE);
}

int ubram_read(int bram_id, unsigned char *buffer, long int size)
{
	if (size > BRAM_SIZE) {
		perror("too big to write\n");
		return 0;
	}

	memcpy(buffer, bram_vptr[bram_id], size);

	return size;
}

int ubram_write(int bram_id, unsigned char *buffer, long int size)
{
	if (size > BRAM_SIZE) {
		perror("too big to write\n");
		return -1;
	}

	memcpy(bram_vptr[bram_id], buffer, size);

	return 0;
}

int ubram_fwrite(int bram_id, int fd, long int size)
{
	int ret = 0;

	if (size > BRAM_SIZE) {
		perror("too big to write\n");
		return 0;
	}

	ret = write(fd, bram_vptr[bram_id], size);
	if (ret < size) {
		perror("write not complete\n");
	}

	return ret;
}
uint32_t chan_fifo_count(uint8_t chan)
{
	uint32_t index = (0x1018 + chan * 0x8)  / 0x8;
	return bram_vptr[1][index] * 0x20;
}
uint8_t index_buf_valid()
{
	uint32_t index = 0x1048 / 0x8;
	return bram_vptr[1][index];
}
uint8_t chan_cut_req(uint8_t chan)
{
	uint32_t index = (0x1030 + chan * 0x8)  / 0x8;
	if (bram_vptr[1][index])
		//printf("chan_cut_req = %d\n", bram_vptr[1][index]);
	return bram_vptr[1][index];
}
void chan_cut_ack(uint8_t chan)
{
	uint32_t index = (0x18 + chan * 0x8) / 0x8;
	bram_vptr[1][index] = 1;
	while (chan_cut_req(chan));		//wait
	bram_vptr[1][index] = 0;
}

void index_buf_invalid()
{
	bram_vptr[1][0x6] = 1;
	while (index_buf_valid());		//wait
	bram_vptr[1][0x6] = 0;
}
int ubram_selftest()
{
	int ret = 0;
//	int i = 0;
	int loop = 0;
	int test_loop = TEST_LOOP_COUNT;
	double duration = 0, speed = 0;
	clock_t start, end;
	int count = 0;
	long long buffer[0x800] = {0};
	int size = BRAM_SIZE/8;


	// 	WRITE TEST
	loop = 0;
	start = clock();
	while (loop < test_loop) {
		loop++;
		for (count = 0; count < size; count+=2)
		{
			bram_vptr[0][count] = 0xDEADBEFFFACEB00C;
			bram_vptr[0][count+1] = 0x55555555AAAA0000 + count;

			bram_vptr[1][count] = 0xFACEB00CDEADBEFF;
			bram_vptr[1][count+1] = 0xAAAAAAAA55550000 + count;
		}
	}
	end = clock();
	duration = (double)(end - start) / CLOCKS_PER_SEC;
	speed = (double)((uint64_t)test_loop  * BRAM_SIZE * 2 / 1024 /1024) / duration;		// MB/s
	printf("BRAM WRITE %u (MB), elapse time: %f (s), speed: %f (MB/s)\n",
			(uint32_t)((uint64_t)test_loop  * BRAM_SIZE * 2 / 1024 /1024), duration, speed);

	// READ TEST
	loop = 0;
	start = clock();
	while (loop < test_loop) {
		loop++;
		memcpy(buffer, bram_vptr[0], BRAM_SIZE);
		memcpy(&buffer[400], bram_vptr[1], BRAM_SIZE);
	}
	end = clock();
	duration = (double)(end - start) / CLOCKS_PER_SEC;
	speed = (double)((uint64_t)test_loop  * BRAM_SIZE * 2 / 1024 /1024) / duration;		// MB/s
	printf("BRAM READ %u (MB), elapse time: %f (s), speed: %f (MB/s)\n",
			(uint32_t)((uint64_t)test_loop  * BRAM_SIZE * 2 / 1024 /1024), duration, speed);


	return 0;
}

