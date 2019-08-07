/*
 * ummap.c
 *
 *  Created on: Mar 23, 2018
 *      Author: terence
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <unistd.h>
#include <fcntl.h>

#include <errno.h>

#include "ummap.h"


#define PAGE_SIZE  ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t) (long)~(PAGE_SIZE - 1))



void* mmap_create(uint32_t phyaddr, size_t len)
{
	int fd = 0;
	void * virtaddr = NULL;
	uint32_t base = phyaddr & PAGE_MASK;

	// open dev/mem
	if((fd = open("/dev/mem", O_RDWR /*| O_NONBLOCK*//*| O_SYNC*/)) == -1)
	{
		printf("%s() failed\n", __func__);
		return NULL;
	}

	virtaddr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
	if(virtaddr == MAP_FAILED)
	{
		printf("%s() failed\n", __func__);
	}

	close(fd);
	return virtaddr;
}

int mmap_release(void *virtaddr, size_t len)
{
	if (virtaddr == NULL)
		return -1;

	munmap(virtaddr, len);

	return 0;
}

//void mmap_iowrite32(uint64_t phyaddr, uint32_t val)
//{
//	int fd;
//	volatile uint8_t *map_base;
//	uint64_t base = phyaddr & PAGE_MASK;
//	uint64_t pgoffset = phyaddr & (~PAGE_MASK);
//
//	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
//	{
//		perror("open /dev/mem:");
//	}
//
//	map_base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
//			fd, base);
//	if(map_base == MAP_FAILED)
//	{
//		perror("mmap:");
//	}
//	*(volatile uint32_t *)(map_base + pgoffset) = val;
//	close(fd);
//	munmap((void *)map_base, PAGE_SIZE);
//}
//
//
//int mmap_ioread32(uint64_t phyaddr)
//{
//	int fd;
//	uint32_t val;
//	volatile uint8_t *map_base;
//	uint64_t base = phyaddr & PAGE_MASK;
//	uint64_t pgoffset = phyaddr & (~PAGE_MASK);
//	//open /dev/mem
//	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
//	{
//		perror("open /dev/mem:");
//	}
//	//mmap
//	map_base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
//			fd, base);
//	if(map_base == MAP_FAILED)
//	{
//		perror("mmap:");
//	}
//	val = *(volatile uint32_t *)(map_base + pgoffset);
//	close(fd);
//	munmap((void *)map_base, PAGE_SIZE);
//
//	return val;
//}


/**
read and write phy mem
 * */
void mmap_write32(void *virtaddr, uint32_t offset, uint32_t value)
{
	*(volatile uint32_t *)(virtaddr + offset) = value;
}

uint32_t mmap_read32(void *virtaddr, uint32_t offset)
{
	return *(volatile uint32_t *)(virtaddr + offset);
}









// ORIG_BUFFER will be placed in memory and will then be changed to NEW_BUFFER
// They must be the same length
#define ORIG_BUFFER "Hello, World!"
#define NEW_BUFFER "Hello, Linux!"

//
//// The page frame shifted left by PAGE_SHIFT will give us the physcial address of the frame
//// Note that this number is architecture dependent. For me on x86_64 with 4096 page sizes,
//// it is defined as 12. If you're running something different, check the kernel source
//// for what it is defined as.
//#define PAGE_SHIFT 12
//#define PAGEMAP_LENGTH 8

//void* create_buffer(void);
//unsigned long get_page_frame_number_of_address(void *addr);
int open_memory(void);
void seek_memory(int fd, unsigned long offset);
int mem_addr_vir2phy(void *vir, uint32_t* phy);

//int main(void) {
//   // Create a buffer with some data in it
//   void *buffer = create_buffer();
//
//   // Get the page frame the buffer is on
//   unsigned int page_frame_number = get_page_frame_number_of_address(buffer);
//   printf("Page frame: 0x%x\n", page_frame_number);
//
//   // Find the difference from the buffer to the page boundary
//   unsigned int distance_from_page_boundary = (unsigned long)buffer % getpagesize();
//
//   // Determine how far to seek into memory to find the buffer
//   uint64_t offset = (page_frame_number << PAGE_SHIFT) + distance_from_page_boundary;
//
//   // Open /dev/mem, seek the calculated offset, and
//   // map it into memory so we can manipulate it
//   // CONFIG_STRICT_DEVMEM must be disabled for this
//   int mem_fd = open_memory();
//   seek_memory(mem_fd, offset);
//
//   printf("Buffer: %s\n", buffer);
//   puts("Changing buffer through /dev/mem...");
//
//   // Change the contents of the buffer by writing into /dev/mem
//   // Note that since the strings are the same length, there's no purpose in
//   // copying the NUL terminator again
//   if(write(mem_fd, NEW_BUFFER, strlen(NEW_BUFFER)) == -1) {
//      fprintf(stderr, "Write failed: %s\n", strerror(errno));
//   }
//
//   printf("Buffer: %s\n", buffer);
//
//   // Clean up
//   free(buffer);
//   close(mem_fd);
//
//   return 0;
//}
void* create_buffer(size_t buf_size) {
//   size_t buf_size = strlen(ORIG_BUFFER) + 1;
//   buf_size = 0x100000;

   // Allocate some memory to manipulate
   //void *buffer = malloc(buf_size);
	void *buffer = NULL;
//	posix_memalign(&buffer, getpagesize(), buf_size);
//   if(buffer == NULL) {
//      fprintf(stderr, "Failed to allocate memory for buffer\n");
//      exit(1);
//   }

	buffer = mmap(NULL, buf_size, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	if(buffer == MAP_FAILED) {
	 fprintf(stderr, "Failed to allocate memory for buffer\n");
	 exit(1);
	}
	madvise(buffer, buf_size, MADV_HUGEPAGE);

   // Lock the page in memory
   // Do this before writing data to the buffer so that any copy-on-write
   // mechanisms will give us our own page locked in memory
   if(mlock(buffer, buf_size) == -1) {
      fprintf(stderr, "Failed to lock page in memory: %s\n", strerror(errno));
      exit(1);
   }

   // Add some data to the memory
   //strncpy(buffer, ORIG_BUFFER, strlen(ORIG_BUFFER));
   //memset(buffer, 0, buf_size);

   return buffer;
}

unsigned long get_page_frame_number_of_address(void *addr) {
   // Open the pagemap file for the current process
   FILE *pagemap = fopen("/proc/self/pagemap", "rb");

   // Seek to the page that the buffer is on
   unsigned long offset = (unsigned long)addr / getpagesize() * PAGEMAP_LENGTH;
   if(fseek(pagemap, (unsigned long)offset, SEEK_SET) != 0) {
      fprintf(stderr, "Failed to seek pagemap to proper location\n");
      exit(1);
   }

   // The page frame number is in bits 0-54 so read the first 7 bytes and clear the 55th bit
   unsigned long page_frame_number = 0;
   fread(&page_frame_number, 1, PAGEMAP_LENGTH-1, pagemap);

   page_frame_number &= 0x7FFFFFFFFFFFFF;

   fclose(pagemap);

   int i = 0;
   uint32_t phy;
   for (i = 0; i < 96; i++)
   {
		if(mem_addr_vir2phy(addr + i * 0x1000, &phy)){
			return -1;
		}
		//printf("%d phy:0x%08x\n", i, phy);
   }

   return page_frame_number;
}

#define PFN_MASK ((((uint64_t)1) << 55) - 1)
#define PFN_PRESENT_FLAG (((uint64_t)1) << 63)


int mem_addr_vir2phy(void *vir, uint32_t* phy)
{
	int fd;
	int page_size = getpagesize();
	unsigned long vir_page_idx = (uint64_t)vir / page_size;
	unsigned long pfn_item_offset = vir_page_idx * sizeof(uint64_t);
	uint64_t pfn_item;

	fd = open("/proc/self/pagemap", O_RDONLY, 777);
	if(fd < 0)
	{
		printf("open pagemap failed\n");
		return -1;
	}

	if((off_t)(-1) == lseek(fd, pfn_item_offset, SEEK_SET))
	{
		printf("lseek pagemap failed\n");
		return -1;
	}

	if(sizeof(uint64_t) != read(fd, &pfn_item, sizeof(uint64_t)))
	{
		printf("read pagemap failed\n");
		return -1;
	}

	if(0 == (pfn_item & PFN_PRESENT_FLAG))
	{
		printf("page is not present\n");
		return -1;
	}

	*phy = (pfn_item & PFN_MASK) * page_size + (uint64_t)vir % page_size;

	close(fd);

	return 0;

}

int open_memory(void) {
   // Open the memory (must be root for this)
   int fd = open("/dev/mem", O_RDWR);

   if(fd == -1) {
      fprintf(stderr, "Error opening /dev/mem: %s\n", strerror(errno));
      exit(1);
   }

   return fd;
}

void seek_memory(int fd, unsigned long offset) {
   unsigned pos = lseek(fd, offset, SEEK_SET);

   if(pos == -1) {
      fprintf(stderr, "Failed to seek /dev/mem: %s\n", strerror(errno));
      exit(1);
   }
}
