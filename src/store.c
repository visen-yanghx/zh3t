/*
 * store.c
 *
 *  Created on: Jun 7, 2018
 *      Author: terence
 *  Store channel data and index data directly.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/statfs.h>

#include "ugpio.h"
#include "ummap.h"
#include "udma.h"
#include "ubram.h"
#include "store.h"
#include "serport.h"

#define IOCTL_DMA_RD		(_IOC(_IOC_READ, 'D', 1, 4))

#define  	   MAX_FILE_COUNT  				1000000
#define  	   PLAYBACK_FILE_COUNT  		4
#define  	   COMPARE_LEN				   32
#define      DMA_SIZE						0x8000
#define		WRITE_MN_FIR			      0x40000
#define		WRITE_MN_MID			      0x80000
#define		WRITE_MN_MID2            0xc0000
#define		WRITE_MN_END			      0x100000
#define		IFD_WRITE_PDW			   0x1000
#define		PDW_WRITE_THRESHOLD		   0x20000
#define		IDX_WRITE_THRESHOLD		   0x2000
#define      DMA_TRANS_SIZE           0x8000//0x8000
#define      DMA_PREREAD              0x80//0x200
#define		IFD_WRITE_MN			      0x60000// DMA_TRANS_SIZE//wf 20181007 0x60000//0x100000
												// 0x60000 384K pdw error



#define		DMA_RAM_1M		   1
#define      DMA_LEHG           0x3000000
#define      STAR_STORE         bram_vptr[1][18]
#define      FILE_MARK          bram_vptr[1][19]
#define      SPEED              bram_vptr[1][20]
#define      SPEED_LS           bram_vptr[1][21]

#define 		LS_STOP_STORE			bram_vptr[1][22]
#define 		MN_STOP_STORE			bram_vptr[1][23]
#define 		SY_STOP_STORE			bram_vptr[1][24]
#define      GPIO               0
#define      cyc_mark           128

extern uint32_t dma_rdable_count[3];
extern uint32_t dma_rd_count[3];
extern uint32_t dma_writable_count[3];
extern uint32_t dma_written_count[3];


volatile uint8_t chan_slice[3] = {0};
volatile uint8_t chan_slice_alt[3] = {1, 1, 1};	// copy from chan_slice. for new file

volatile uint8_t chan0_index = 0;		// indicates ending indexing
volatile uint8_t chan0_index_alt = 1;	// indicates beginning to index

volatile uint8_t chan_stored[3] = {0};
volatile uint8_t chan0_indexed = 0;
volatile uint8_t chan_playback[3] = {1, 1, 1};

volatile uint32_t fd[2] = {0,0};
volatile uint32_t chan1_read_bytes = 0;
volatile uint32_t write_bytes = 0;
volatile uint32_t read_pos = 0;
volatile uint32_t pdw_read_byte = 0;
volatile uint32_t write_pos = 0;
volatile uint32_t time_bram= 0;

volatile uint32_t  time_msec= 0;
volatile uint32_t  time_sec= 0;
volatile uint32_t  time_min= 0;
volatile uint32_t  time_hor= 0;
volatile uint32_t  time_day= 0;
volatile uint32_t  time_mon= 0;
volatile uint32_t  time_yer= 0;

volatile unsigned long long  sys_file_name[16];

volatile uint32_t  new_file_mark= 1;
             char  file_name_all[cyc_mark+1][256]= {0};
volatile uint32_t  time_sec1= 0;
volatile uint32_t  time_min1= 0;
volatile uint32_t  time_hor1= 0;
volatile uint32_t  time_day1= 0;
volatile uint32_t  time_mon1= 0;
volatile uint32_t  time_yer1= 0;
volatile uint32_t clear = 1;
volatile uint32_t num_ls= 1;
volatile uint32_t num_mn= 1;
volatile uint32_t zero_mark=0;

void *data_buffer       = NULL;
void *data_buffer_begin = NULL;
void *data_buffer_end   = NULL;

typedef struct _axidma_buffer {
	   char   rev_mark;
      char   rev_num;
 }dma_buffer;
uint32_t index_count = 0;
uint8_t index_end_identifier[COMPARE_LEN] = {0};
uint8_t chan_end_identifier[COMPARE_LEN] =  {0};
uint32_t   buffer_mark = 1;
dma_buffer dma_buff1;
dma_buffer dma_buff2;
dma_buffer dma_buff3;
dma_buffer dma_buff4;
volatile int ls_file_num = 0;
volatile int mn_file_num = 0;
volatile int sy_file_num = 0;
char file_name_sys[MAX_DIR_NAME_LENGTH+64] = {0};
//char recordDirName[MAX_DIR_NAME_LENGTH];
// record routines
static void wait_slice();
static int chan_recv_routine(int chan_num);
static int chan1_recv(int chan_num);
static int chan0_recv_routine(void);
static int chan1_recv_routine(void);

static int index_recv_routine(void);
static int CreateDir (const char *sPathName);
extern void net_routine();
       void start_heartbeat();

int store_init()
{
    gpio_init(80);		// GPIO MUST FIRST due to controls use GPIO.
    reset_pl();			// Then reset PL

    // Other module initialization
    ubram_init();

    STAR_STORE = 0;
    bram_vptr[1][19] = 1;
    dma_buff1.rev_mark = 0 ;
    dma_buff2.rev_mark = 0 ;
    dma_buff3.rev_mark = 0 ;
    dma_buff4.rev_mark = 0 ;
    dma_buff1.rev_num  = 0 ;
    dma_buff2.rev_num  = 0 ;
    dma_buff3.rev_num  = 0 ;

    sleep(0.5);
    posix_memalign(&data_buffer_begin, getpagesize(), 0x3000000);
    posix_memalign(&data_buffer_end,   getpagesize(), 0x3000000);
    dma_init();
    // Settings for data source. include frame length, count, mode etc.
    use_dma_buf_flag(0);      //1  use   0 no use
    select_frame_length(0);
    select_frame_count(3);
    select_data_source(0);    //1 test   0 aurora
    check_en(1);    //1 on   0 off
    // Enable data source after settings
    enable_data_source(0);    //1 on   0 off

	return 0;
}

int read_time()
{
	 unsigned char *time_buffer = malloc(0x100000);		//wufan 20180827
	 time_bram =  ubram_read(1, &time_buffer[0], 0x2000);
	 time_sec = time_buffer[4104];
	 time_min = time_buffer[4105];
	 time_hor = time_buffer[4106];
	 time_day = time_buffer[4107];
	 time_mon = time_buffer[4108];
	 time_yer = time_buffer[4109];
	 return 0;

}

static void wait_slice()
{

	if (chan_cut_req(0)) {
		 chan_cut_ack(0);
		chan_slice[0] = chan0_index = 1;
		//num_mn++;
		printf("%d()chan0 slice\n", num_mn++);
		//printf("\n%s()chan0 slice\n", __func__);
	    }
	if (chan_cut_req(1)) {
		//write_bytes = pdw_read_byte;
		//pdw_read_byte = 0;
		//read_pos = 0;
		chan_cut_ack(1);
		chan_slice[1] = 1;
		//num_ls++;
		printf("%d()chan1 slice\n", num_ls++);
		//printf("\n%s()chan1 slice\n", __func__);
	}


}

void store_control()
{
	int slice_count[3] = {1, 1, 1};
	while (1) {
    	// 等待接收线程全部准备好后，使能数据源
    		if (chan0_indexed && chan_stored[0]) {
    		//	if (chan_stored[0]) {
    			if (slice_count[0] < MAX_FILE_COUNT) {
    				//printf("%s() chan0 req:%d\n", __func__, flow_ctrl_req(0));
		    		if (flow_ctrl_req(0)) {
		    			//printf("chan0_flow_over\n");
		    			flow_ctrl_ack(0);
		    			//printf("%s() chan0_flow_req\n", __func__);
		    			chan0_index_alt = chan_slice_alt[0] = 1;
		    			chan0_indexed = chan_stored[0] = 0;
		    			slice_count[0]++;
		    		}
    			}
    		}
    		// chan1
    		if (chan_stored[1]) {
    			if (slice_count[1] < MAX_FILE_COUNT) {
    				chan_slice_alt[1] = 1;
    				chan_stored[1] = 0;
    				slice_count[1]++;
    			}
    		}

    	// check file slice
    	usleep(10);
    	wait_slice();
    }
}

///////////////////////////// FOR DEBUG //////////////////////////////
unsigned char *gdmadata[10];
unsigned char gdmacount = 0;
///////////////////////////// FOR DEBUG //////////////////////////////
/**********************************
 * @return,  return 0 if test successfully. Else return -1.
 */
int store_prepare_recv()
{
	pthread_t  index_tid;
	pthread_t  chan0_rx_read;
	pthread_t  chan1_rx_read;
	pthread_t  net;

	// 启动数据接收线程
	pthread_create(&chan0_rx_read,  NULL, (void*)chan0_recv_routine,  NULL);

	pthread_create(&chan1_rx_read,  NULL, (void*)chan1_recv_routine,  NULL);

	pthread_create(&index_tid, NULL, (void*)index_recv_routine, NULL);

    pthread_create(&net, NULL, (void*)net_routine, NULL);
	return 0;
}


static int index_recv_routine()
{

    int fd = 0;
   // int sy_file_num_cyc;
    char file_name[128];
    uint32_t  recv_bytes = 0;
    uint32_t ret_bytes = 0;
    unsigned char *index_buffer = malloc(0x100000);


    while (1) {

		if(SY_STOP_STORE == 1)
		  {
			  usleep(500);

			  if((zero_mark == 1) && (sy_file_num == ls_file_num ))
			   remove(file_name);
				recv_bytes = 0;
				ret_bytes = 0;
				chan0_index = 0;
				chan0_index_alt = 1;
				chan0_indexed = 1;
				SY_STOP_STORE = 0;

				close(fd);
				printf("close sy_file .\n");
		      // }
		   }
      if (!chan0_indexed) {
				if (index_buf_valid()) {
					index_count++;
					// copy data from BRAM to DDR
					ret_bytes =  ubram_read(0, &index_buffer[recv_bytes], BRAM_SIZE);
					//invalidate_bram();		// notify bram data is invalid and can be covered
					index_buf_invalid();
				}
		        // 新建文件
				 if (chan0_index_alt && (!FILE_MARK) ) {
					 sprintf(file_name, "%s.bxsldsy",file_name_all[(sy_file_num%cyc_mark)]);
					 sy_file_num++;
					 fd = open(file_name, O_RDWR | O_CREAT  | O_TRUNC, MODE);
		        	if(fd < 0){
		        		printf("%s() Failed to Create file %s.\n", __func__, file_name);
		        		continue;
		        	} else {
		        		chan0_index_alt = 0;
		    			recv_bytes = 0;
		        		printf("%s() New file %s.\n", __func__, file_name);
		        	}
		        }

		        // 保存数据
				recv_bytes += ret_bytes;
				if (recv_bytes == IDX_WRITE_THRESHOLD) {
					if(chan0_index_alt == 0){
						write(fd, index_buffer, IDX_WRITE_THRESHOLD);		// 保存数据	//9
					}
					recv_bytes = 0;
					ret_bytes  = 0;															//wf 20180821
				}

				if (chan0_index == 1) {

				if ((recv_bytes == 0) || !memcmp(&index_buffer[recv_bytes], index_end_identifier, COMPARE_LEN))
				{
					//write(fd, index_buffer, recv_bytes);		// 保存数据
					close(fd);
					recv_bytes = 0;
					ret_bytes = 0;
					chan0_index = 0;
					chan0_index_alt = 1;
					chan0_indexed = 1;

				}
			}
		}
    }

	return 0;
}

static int chan_recv_routine(int chan_num)
{
	char file_name[128] = {0};

	uint32_t read_bytes = 0;
	uint32_t file_read_bytes = 0;
	uint32_t ret_bytes = 0;
   struct timeval start_time, end_time;
	uint64_t write_disk_bytes = 0;
   double duration = 0.0f, speed = 0.0f;
	while (1) {
		//go to idle
	  if(MN_STOP_STORE == 1)
	  {
	          usleep(500);

	          if((zero_mark == 1) && (mn_file_num == ls_file_num ))
			   remove(file_name);
				read_bytes = 0;
				ret_bytes = 0;
				chan_slice[chan_num] = 0;
				chan_stored[chan_num] = 1;
				file_read_bytes = 0;
				write_disk_bytes = 0;

				MN_STOP_STORE = 0;
				close(fd[chan_num]);
				printf("close mn_file .\n");

	    }
		if (!chan_stored[chan_num]) {
			// 新建文件
				if ((chan_slice_alt[chan_num]) && (!FILE_MARK)) {

				sprintf(file_name, "%s.bxsldmn",file_name_all[(mn_file_num%cyc_mark)]);
				mn_file_num++;
				fd[chan_num] = open(file_name, FLAGS, MODE);
				if(fd[chan_num] < 0){
					printf("%s() Failed to Create file %s.\n", __func__, file_name);
					continue;
				} else {
					chan_slice_alt[chan_num] = 0;

					printf("New chan%d file %s.\n",chan_num, file_name);
					FILE_MARK = 1;
					gettimeofday(&start_time, NULL);
				}
			}

			 if((chan_fifo_count(0) >= (DMA_TRANS_SIZE - DMA_PREREAD)) && (!chan_slice_alt[chan_num])){
				ret_bytes = dma_ioctl_read(axidma[chan_num].dma_num, axidma[chan_num].fd,read_bytes, DMA_SIZE);
				file_read_bytes += ret_bytes;
				read_bytes += ret_bytes;
			}

			if (read_bytes == IFD_WRITE_MN) {
				if(chan_slice_alt[chan_num] == 0){
     			write(fd[chan_num], axidma[chan_num].rxmem_vbase, IFD_WRITE_MN);     //9
     			//sync();
				}
				read_bytes = 0;
				write_disk_bytes = write_disk_bytes + IFD_WRITE_MN;
			}
		  //cut
			if (chan_slice[chan_num]) {
				if (read_bytes > 0) {
					write(fd[chan_num], axidma[chan_num].rxmem_vbase, read_bytes);	//9
					//sync();
					write_disk_bytes = write_disk_bytes + (uint64_t)read_bytes;
				}
	          close(fd[chan_num]);
				gettimeofday(&end_time, NULL);
				duration = calc_duration(start_time, end_time);
				speed = ((double)((uint64_t)write_disk_bytes / 1024 /1024)) / duration;		// MB/s
				printf("CHAN%d READ %u (MB), elapse time: %f (s), speed: %f (MB/s)\n", chan_num,
						(uint32_t)((uint64_t)write_disk_bytes/1024/1024), duration, speed);
				SPEED  =  (unsigned long long )(speed*1000);
				read_bytes = 0;
				ret_bytes = 0;
				chan_slice[chan_num] = 0;
				chan_stored[chan_num] = 1;
				file_read_bytes = 0;
				write_disk_bytes = 0;

		}

		}

	}

	return 0;
}
static int chan1_recv(int chan_num)
{
	uint32_t ret_bytes = 0;
	uint32_t read_bytes = 0;

	struct timeval start_time,end_time;
	double duration = 0 ,speed = 0.0f;
	int   fd ;
	char file_name[128]      = {0};
	unsigned char *ad_buffer = malloc(0x100000);
	unsigned char file_buffer[1280] = {0};
	while (1)
	{

		if(1 == STAR_STORE)
		{
			read_bytes = 0;
			ret_bytes = 0;
			zero_mark = 0;
			sprintf(file_name_sys,"%s","/smbshare/");
			time_bram =  ubram_read(1, &ad_buffer[0], 0x2000);	//0x82001008
			time_bram =  ubram_read(1, &file_buffer[0], 0x300);	//0x82001008
			time_sec  = ad_buffer[4104];
			time_min  = ad_buffer[4105];
			time_hor  = ad_buffer[4106];
			time_day  = ad_buffer[4107];
			time_mon  = ad_buffer[4108];
			time_yer  = ad_buffer[4109];

			memcpy(&file_name_sys[10],&ad_buffer[240],256);

			sprintf(file_name_sys,"%s/",file_name_sys);
			CreateDir(file_name_sys);
			printf("MAKE DIR:%s\r\n",file_name_sys);
			ls_file_num = 0;
			sprintf(file_name_all[ls_file_num],"%s20%02x%02x%02x%02x%02x%02x_%d",file_name_sys,time_yer,time_mon,time_day,time_hor,time_min,time_sec,ls_file_num);
			sprintf(file_name, "%s.bxsldpls",file_name_all[ls_file_num]);
			printf("RTC:20%02x%02x%02x%02x%02x%02x\r\n",time_yer,time_mon,time_day,time_hor,time_min,time_sec);
			fd = open(file_name, FLAGS, MODE);
			if(fd < 0)
			{
				printf("%s() Failed to Create file %s.\n", __func__, file_name);
				continue;
			}
			else
			{
				printf(" New chan%d file %s.\n",  chan_num, file_name);
				sy_file_num = ls_file_num;
				mn_file_num = ls_file_num;
				ls_file_num++;

				usleep(10);
				STAR_STORE = 0;
				FILE_MARK = 0;

				printf("STAR_STORE:%lld \n", bram_vptr[1][18]);
			}
		}

	   if( 1 == LS_STOP_STORE )
		{

			   printf("LS_STOP_STORE2:%lld \n", bram_vptr[1][22]);
				off_t fd_length = lseek(fd,0,SEEK_END);
				printf(" fd_length = %ld\n",fd_length);
				if(fd_length == 0)
				{
					zero_mark  = 1;
					if(remove(file_name) == 0 )
						printf("remove %s\n",file_name);
					else
						printf("remove error");
				}else if(ls_file_num > ls_file_num )
				{
					if(remove(file_name) == 0 )
						printf("remove %s\n",file_name);
					else
						printf("remove error");
				}
				printf("close ls_file .\n");

				LS_STOP_STORE = 0;
				printf("LS_STOP_STORE3:%lld \n", bram_vptr[1][22]);
				close(fd);

		}


		if(!chan_stored[chan_num])
		{
			if (chan_fifo_count(chan_num) >= (DMA_TRANS_SIZE - DMA_PREREAD))
			{
			   gettimeofday(&start_time, NULL);
				ret_bytes = dma_ioctl_read(axidma[chan_num].dma_num, axidma[chan_num].fd, read_bytes, DMA_TRANS_SIZE);
				read_bytes += ret_bytes;
			}
			// 保存数据
			if (read_bytes == IFD_WRITE_MN)
			{		//IFD_WRITE_MN
				if(fd>0)
				{
					write(fd, axidma[chan_num].rxmem_vbase, IFD_WRITE_MN);	//9
				}
				read_bytes = 0;
			}
		   gettimeofday(&end_time, NULL);


			// 处理最后几帧数
		   if (chan_slice[chan_num])
			{
				if (read_bytes > 0)
				{
					write(fd, axidma[chan_num].rxmem_vbase, read_bytes);	//9
				}
				off_t length = lseek(fd, 0x0, SEEK_END);
				write(fd, axidma[chan_num].rxmem_vbase, IFD_WRITE_MN);
				ftruncate(fd,length);
				close(fd);

				duration = calc_duration(start_time, end_time);
				speed = ((double)((uint64_t)IFD_WRITE_MN / 1024 )) / duration;		// MB/s
				printf("CHAN%d READ %u (KB), elapse time: %f (s), speed: %f (MB/s)\n", chan_num,
						(uint32_t)((uint64_t)IFD_WRITE_MN/1024), duration, speed/1024);
				bram_vptr[1][21]  =  (unsigned long long )speed;

				time_bram =  ubram_read(1, &ad_buffer[0], 0x2000);	//0x82001008
				time_sec  = ad_buffer[4104];
				time_min  = ad_buffer[4105];
				time_hor  = ad_buffer[4106];
				time_day  = ad_buffer[4107];
				time_mon  = ad_buffer[4108];
				time_yer  = ad_buffer[4109];
				sprintf(file_name_all[(ls_file_num%cyc_mark)],"%s20%02x%02x%02x%02x%02x%02x_%d",file_name_sys,time_yer,time_mon,time_day,time_hor,time_min,time_sec,ls_file_num);
				sprintf(file_name, "%s.bxsldpls",file_name_all[(ls_file_num%cyc_mark)]);
				ls_file_num++;
				fd = open(file_name, FLAGS, MODE);
				if(fd < 0)
				{
					printf("%s() Failed to Create file %s.\n", __func__, file_name);
					continue;
				}
				else
				{
					printf(" New chan%d file %s.\n", chan_num, file_name);
					FILE_MARK = 0;
				}
				read_bytes = 0;
				ret_bytes = 0;
				chan_slice[chan_num] = 0;
				chan_stored[chan_num] = 1;

			}
		}

	}
	return 0;
}

static int chan0_recv_routine()
{

   while (1) {
	   if (flow_ctrl_req(0)) {
  		    flow_ctrl_ack(0);
  		    break;
  	         }
         }
	chan_recv_routine(0);
	return 0;
}

static int chan1_recv_routine()
{
	  chan1_recv(1);
	return 0;
}

int  CreateDir (const char *sPathName)
	{
	   char  DirName[256];
	   strcpy(DirName,sPathName);
	   int i,len = strlen(DirName);
	   for(i = 1 ;i < len ; i++)
	   {
	        if(DirName[i]=='/')
	          {
	           DirName[i] = 0;
	              if(access(DirName,NULL)!=0)
	                    {
	                   if(mkdir(DirName,0777) == -1)
	                         {
	                          printf("mkdir error\n");
	                          return -1;
	                          }
	                      chmod(DirName ,0777);
	                    }
	           DirName[i] = '/';
	          }
	   }
	      return 0;
	}

static int heartbeat_routine()
{
	struct statfs mdstat;
	int eth = 0;
	char ethchk_cmd[128];

	int fd = serial_open("/dev/ttyPS1");
	if (fd < 0) {
		return -1;
	}

	if (serial_init(fd) < 0)
		return -1;

	while (1) {
		// system run status
		serial_send(fd, "1\r\n", 3);

		// sata status
		if (-1 == statfs("/dev/md127p1", &mdstat)) {
			serial_send(fd, "2\r\n", 3);
		}

		sleep(10);
	}

	return 0;
}

void start_heartbeat()
{
	pthread_t tid;
	pthread_create(&tid, NULL, (void*)heartbeat_routine, NULL);
}

