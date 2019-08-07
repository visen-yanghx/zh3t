/*
typedef enum _ZH3T_CMD_
{
	CMD_GET_STATUS = 1,
	CMD_GET_VOLT_TEMP = 2,
	CMD_SET_START = 3,
	CMD_SET_STOP = 4,
	CMD_SET_RTC = 5,
	CMD_GET_RTC = 6,

	ACK_GET_STATUS = 21,
	ACK_GET_VOLT_TEMP = 22,
	ACK_SET_START = 23,
	ACK_SET_STOP = 24,
	ACK_SET_RTC = 25,
	ACK_GET_RTC = 26
}ZH3T_CMD;
*/

#include <stdio.h>
#include <struct_tm.h>
#include <time_t.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include "ubram.h"
#include <sys/vfs.h>
#include <net/if.h>
#include "ugpio.h"
#include "store.h"
#include <fcntl.h>
#include <stdlib.h>    //20190710
#include <dirent.h>
#include <errno.h>


#define   MAX            256
#define   ACK_SIZE       32
#define   BUFFERSIZE     300


volatile int fpga_temp    = 0x123456;
volatile int fpga_corev   = 0x123456;
volatile int fpga_assv    = 0x123456;
volatile int fpga_bt      = 0x123456;
volatile int ppc_temp     = 0x123456;
volatile int V7fpga_temp  = 0x123456;
volatile int fisk_warn    = 0;
volatile int fisk_threshold = 0;
volatile char path[MAX];
extern     uint16_t  duration ;
extern	     uint16_t  speed ;
extern    unsigned long long *bram_vptr[2];
extern void enable_data_source(uint8_t value);
extern void select_data_source(uint8_t mode);
       void set_datetime(int year, int mon, int mday, int hour, int  min, int sec);
       int get_file_count(char *root);
int connect_check();

struct TIME
{
	char  TIME_msec;
	char  TIME_sec;
	char  TIME_min;
	char  TIME_hour;
	char  TIME_day;
	char  TIME_mon;
	char  TIME_year;
} ;
typedef enum DEVICE_WORK_TYPE
{
	WORK_EMPTY    = 0,      /* 鏃犳晥   00000000*/
	WORK_READY    = 1,      /* 灏辩华   00000001*/
	WORK_RECORD   = 2,      /* 璁板綍   00000010*/
	WORK_REPLAY   = 4,      /* 鍥炴斁   00000100*/
	WORK_IMPORT   = 8,      /* 瀵煎叆   00001000*/
	WORK_EXPORT   = 16,     /* 瀵煎嚭   00010000*/
	WORK_ERROR    = 32,     /* 鏁呴殰   00100000*/
	WORK_DISKFULL = 64,     /* 鐩樻弧   01000000*/
	WORK_FIX      = 128,    /* 瀹氭椂璁板綍鎴栧畾闀胯锟�?? */
	WORK_CIRCLE   = 256,    /* 寰幆瑕嗙洊璁板綍 */
   WORK_SHNAP    = 512,    /* 蹇収   00100000*/
   WORK_UPANDDN  = 1024,   /* 涓婁笅锟�?? 10000000*/
}DEVICE_WORK_TYPE;
#define   EXT3_SUPEP_MAGIC     0xEF53
#define   BRAM_SIZE_RECIVE     0x2000

/* deal*/
void process_conn_server(int s)
{
	 ssize_t size = 0;
	 struct TIME  my_time;
	 struct statfs sfs;
	 unsigned char *recive_buffer = malloc(0x2000);
	 int  i,j,time_bram,lan_statue;
	 unsigned long long disk_size,avilable_size;
	 uint32_t start_record = 0,new_file = 0;
	 uint32_t total;
	 enum  DEVICE_WORK_TYPE   device_state = WORK_READY;

	 char buffer[BUFFERSIZE] = {0};	/* buff */
	 unsigned char *time_buffer = malloc(0x100000);
	 for(;;)
	 {
		usleep(1000);
		i = statfs("/smbshare",&sfs);
		if(sfs.f_type != EXT3_SUPEP_MAGIC)
		{/* err */
			return;
		}
		disk_size = (sfs.f_blocks * sfs.f_bsize) ;   //byte

		disk_size = 0x2ba7def3000;

		if (((sfs.f_blocks - sfs.f_bfree)* sfs.f_bsize) <= 0x2ba7def3000 )
			avilable_size = 0x2ba7def3000 - (sfs.f_blocks - sfs.f_bfree)* sfs.f_bsize;
		else
			avilable_size = 0x0;
//		avilable_size = sfs.f_bfree * sfs.f_bsize;
		if ( avilable_size > 0x2ba7def3000 )
			avilable_size = 0x2ba7def3000;
//		else if ( sfs.f_bfree <= 0x100000 )
//			avilable_size = 0x0;

		if(avilable_size < (30000000000 * fisk_threshold))
		{
			fisk_warn = 0x1 ;
			gpio_set(62, 0);//1
		}
		else
		{
			fisk_warn = 0x0 ;
			gpio_set(62, 1);//1
		}

		if ( avilable_size == 0x0 )
		{
			if ( bram_vptr[1][25]  == WORK_RECORD )
			{
				gpio_set(64, 1);//1
				start_record = 0;
				bram_vptr[1][9]   = 1 ;           //0x82000040
				enable_data_source(0);
			}
			bram_vptr[1][25]  = WORK_DISKFULL;
		}
		else
		{
			if ( bram_vptr[1][25]  != WORK_RECORD )
				bram_vptr[1][25]   = WORK_READY ;
		}

		/* read the data to buff */
		size = read(s, buffer, 1024);	

		if(size == 0)
		{/* err */
			return;
		}

		switch (buffer[3]){
			case  0x01:               //CMD_GET_STATUS
			   //sfs.f_type          sys_type  0xEF53 EXT3_SUPEP_MAGIC
			   //sfs.f_blocks        all   block---0x2b88c4b5---730383541*4K/1024/1024/1024=2.72T
			   //sfs.f_bfree         all   free----0x2b841af2
			   //sfs.f_bavail    not root  free----0x294db7cc

   		   // avilable_size = (sfs.f_bavail* sfs.f_bsize) + 0x255c774000;//byte


				buffer[0] = 0x7e;
				buffer[1] = 0x7e;

				buffer[2] = 0x00;
				buffer[3] = 0x21;

				buffer[4] = 0x00;                       //work_status  -work_empty-work_readwork_recordwork_replaywork_exportwork_errorwork_diskfullwork_fixwork_circlework_shnap
				buffer[5] = 0x00;

				//printf(" sfs.f_blocks = %lx, sfs.f_bfree = %lx, disk_size = %llx, avilable_size = %llx \n ", sfs.f_blocks, sfs.f_bfree, disk_size, avilable_size  );
				//printf(" device_state = %x\n ", device_state );

				buffer[6] = ( bram_vptr[1][25]  >> 8 ) & 0xff;
				buffer[7] =  bram_vptr[1][25]   		  & 0xff;

				buffer[8]  = (disk_size >> 56)  & 0xff ;    //disk_size
				buffer[9]  = (disk_size >> 48 ) & 0xff ;
				buffer[10] = (disk_size >> 40 ) & 0xff ;
				buffer[11] = (disk_size >> 32 ) & 0xff;
				buffer[12] = (disk_size >> 24 ) & 0xff;
				buffer[13] = (disk_size >> 16 ) & 0xff;
				buffer[14] = (disk_size >> 8 )  & 0xff;
				buffer[15] =  disk_size         & 0xff;

				buffer[16] = (avilable_size >> 56)  & 0xff;   //avilable_size
				buffer[17] = (avilable_size >> 48)  & 0xff;
				buffer[18] = (avilable_size >> 40)  & 0xff;
				buffer[19] = (avilable_size >> 32)  & 0xff;
				buffer[20] = (avilable_size >> 24)  & 0xff;
				buffer[21] = (avilable_size >> 16)  & 0xff;
				buffer[22] = (avilable_size >> 8)   & 0xff;
				buffer[23] = avilable_size          & 0xff;

				buffer[24] = fisk_warn;
				buffer[25] = fisk_warn;
				buffer[26] = 0x00;
				buffer[27] = 0x00;
				buffer[28] = 0x00;
				buffer[29] = 0x00;

				buffer[30] = 0xfd;
				buffer[31] = 0xfd;
				write(s, buffer, ACK_SIZE );

				break;

			case  0x02:                   //CMD_GET_VOLT_TEMP
				memcpy(recive_buffer, bram_vptr[1], BRAM_SIZE_RECIVE); //0x820001000
				lan_statue = connect_check();
				buffer[0] = 0x7e;
				buffer[1] = 0x7e;
				buffer[2] = 0x00;
				buffer[3] = 0x22;

				buffer[4] =   0x0;     //fpga_temp-temp_fpga
				buffer[5] =   0x0;
				buffer[6] =  (recive_buffer[4113] ) & 0x03;  //(recive_buffer[4329] ) & 0x03;
				buffer[7] =  (recive_buffer[4112] ) & 0xff;  //(recive_buffer[4328] ) & 0xff;

				buffer[8] =   0x0;    //fpga_corev-volt_fpga
				buffer[9] =   0x0;
				buffer[10] =  (recive_buffer[4117] ) & 0x03;  //(recive_buffer[4333]  ) & 0x03;
				buffer[11] =  (recive_buffer[4116] ) & 0xff;  //(recive_buffer[4332]  ) & 0xff ;

				buffer[12] = (fpga_assv >> 24 ) & 0xff;     //fpga_assv-volt_auxi
				buffer[13] = (fpga_assv >> 16 ) & 0xff;
				buffer[14] = (fpga_assv >> 8 )  & 0xff;
				buffer[15] =  fpga_assv         & 0xff;

				buffer[16] = (fpga_bt >> 24 ) & 0xff;       //fpga_bt-temp_board
				buffer[17] = (fpga_bt >> 16 ) & 0xff;
				buffer[18] = (fpga_bt >> 8 )  & 0xff;
				buffer[19] =  fpga_bt         & 0xff;

				buffer[20] = (ppc_temp >> 24 ) & 0xff;      //ppc_temp-temp_ppc
				buffer[21] = (ppc_temp >> 16 ) & 0xff;
				buffer[22] = (ppc_temp >> 8 )  & 0xff;
				buffer[23] =  ppc_temp         & 0xff;

				buffer[24] = (V7fpga_temp >> 24 ) & 0xff;   //V7fpga_temp-temp_v7
				buffer[25] = (V7fpga_temp >> 16 ) & 0xff;
				buffer[26] = (V7fpga_temp >> 8 )  & 0xff;
				buffer[27] =  V7fpga_temp         & 0xff;

				buffer[28] = recive_buffer[4110] ;          //channel
				buffer[29] = lan_statue  <<4 ;              //lan

				buffer[30] = 0xfd;
				buffer[31] = 0xfd;
				write(s, buffer, ACK_SIZE);
				break;
			case  0x03:                   //CMD_SET_START
				if ( bram_vptr[1][25]  == WORK_READY )
				{
					bram_vptr[1][25]  = WORK_RECORD;
					reset_pl();
					bram_vptr[1][20]  = 0;
				   bram_vptr[1][22]  = 0 ;
				   bram_vptr[1][23]  = 0 ;
				   bram_vptr[1][24]  = 0 ;
					gpio_set(64, 0);//1
					start_record = 0x1;
					new_file     = 0x1;
					my_time.TIME_year =  ((buffer[5]/10) <<  4)  | (buffer[5]%10);  //y
					my_time.TIME_mon  =  ((buffer[7]/10) <<  4)  | (buffer[7]%10);
					my_time.TIME_day  =  ((buffer[9]/10) <<  4)  | (buffer[9]%10);
					my_time.TIME_hour =  ((buffer[11]/10) << 4)  | (buffer[11]%10);
					my_time.TIME_min  =  ((buffer[13]/10) << 4)  | (buffer[13]%10);
					my_time.TIME_sec  =  ((buffer[15]/10) << 4)  | (buffer[15]%10);
					my_time.TIME_msec =  ((buffer[17]/10) << 4)  | (buffer[17]%10); //s

					bram_vptr[1][12]   = (unsigned long long)my_time.TIME_sec  ;  //0x82000010
					bram_vptr[1][13]   = (unsigned long long)my_time.TIME_min  ;  //0x82000018
					bram_vptr[1][14]   = (unsigned long long)my_time.TIME_hour ;  //0x82000020
					bram_vptr[1][15]   = (unsigned long long)my_time.TIME_day  ;  //0x82000028
					bram_vptr[1][16]   = (unsigned long long)my_time.TIME_mon  ;  //0x82000030
					bram_vptr[1][17]   = (unsigned long long)my_time.TIME_year ;  //0x82000038

					memcpy(&bram_vptr[1][30], &buffer[18], 256);
					time_buffer[0]   = my_time.TIME_sec  ;
					time_buffer[1]   = my_time.TIME_min  ;
					time_buffer[2]   = my_time.TIME_hour ;
					time_buffer[3]   = my_time.TIME_day  ;
					time_buffer[4]   = my_time.TIME_mon  ;
					time_buffer[5]   = my_time.TIME_year ;
					time_buffer[6]   = 0x0;
					time_buffer[7]   = 0x0;
					//校正系统时间
				   set_datetime(buffer[5],buffer[7],buffer[9],buffer[11],buffer[13],buffer[15]);
					//RTC矫正一次时间：190121
					bram_vptr[1][1]  =  0x0;
					memcpy( &bram_vptr[1][0], time_buffer, 0x8);
					usleep(10);
					bram_vptr[1][1]  =  0x1;
					usleep(10);
					bram_vptr[1][1]  =  0x0;
					//再开始建立文件夹
					bram_vptr[1][18]   = (unsigned long long)start_record ;       //0x82000040
					bram_vptr[1][19]   = (unsigned long long)new_file ;           //0x82000048
					enable_data_source(1);
				}
				buffer[0] = 0x7e;
				buffer[1] = 0x7e;

				buffer[2] = 0x00;
				buffer[3] = 0x23;

				buffer[28] = 0xcc;
				buffer[29] = 0xcc;

				buffer[30] = 0xfd;
				buffer[31] = 0xfd;
				write(s, buffer, ACK_SIZE);
				break;
			case  0x04:                   //CMD_SET_STOP
				bram_vptr[1][25]   = WORK_READY ;
				start_record = 0;
 	      		 gpio_set(64,1);
				bram_vptr[1][9]   = 1 ;           //0x82000040
 	       		bram_vptr[1][20]  = 0 ;
 	      		 bram_vptr[1][21]  = 0 ;

 	      		 bram_vptr[1][22]  = 1 ;
 	       		bram_vptr[1][23]  = 1 ;
 	       		bram_vptr[1][24]  = 1 ;
				enable_data_source(0);

				buffer[0] = 0x7e;
				buffer[1] = 0x7e;

				buffer[2] = 0x00;
				buffer[3] = 0x24;

				buffer[28] = 0xdd;
				buffer[29] = 0xdd;

				buffer[30] = 0xfd;
				buffer[31] = 0xfd;
				write(s, buffer, ACK_SIZE);
				break;
			case  0x05:                   //CMD_SET_RTC
				my_time.TIME_year  =  ((buffer[5]/10) <<  4)  | (buffer[5]%10);  //y
				my_time.TIME_mon   =  ((buffer[7]/10) <<  4)  | (buffer[7]%10);
				my_time.TIME_day   =  ((buffer[9]/10) <<  4)  | (buffer[9]%10);
				my_time.TIME_hour  =  ((buffer[11]/10) << 4)  | (buffer[11]%10);
				my_time.TIME_min   =  ((buffer[13]/10) << 4)  | (buffer[13]%10);
				my_time.TIME_sec   =  ((buffer[15]/10) << 4)  | (buffer[15]%10);
				my_time.TIME_msec  =  ((buffer[17]/10) << 4)  | (buffer[17]%10); //s
				time_buffer[0]     = my_time.TIME_sec  ;
				time_buffer[1]     = my_time.TIME_min  ;
				time_buffer[2]     = my_time.TIME_hour ;
				time_buffer[3]     = my_time.TIME_day  ;
				time_buffer[4]     = my_time.TIME_mon  ;
				time_buffer[5]     = my_time.TIME_year ;
				time_buffer[6]     = 0x0;
				time_buffer[7]     = 0x0;
				//校正系统时间
			   set_datetime(buffer[5],buffer[7],buffer[9],buffer[11],buffer[13],buffer[15]);

				buffer[0] = 0x7e;
				buffer[1] = 0x7e;

				buffer[2] = 0x00;
				buffer[3] = 0x25;

				buffer[28] = 0xee;
				buffer[29] = 0xee;

				buffer[30] = 0xfd;

				buffer[31] = 0xfd;
				write(s, buffer, ACK_SIZE);
				bram_vptr[1][1]  =  0x0;
				memcpy( &bram_vptr[1][0], time_buffer, 0x8);
				usleep(1000);
				bram_vptr[1][1]  =  0x1;
				usleep(1000);
				bram_vptr[1][1]  =  0x0;
				break;
			case  0x06:                       //CMD_GET_RTC
				time_bram =  ubram_read(1, &time_buffer[0], 0x2000);	//0x820010f0
				buffer[0] = 0x7e;
         	    		buffer[1] = 0x7e;

         	   		 buffer[2] = 0x00;
         	   		 buffer[3] = 0x26;

				buffer[5] = ((time_buffer[4109] & 0xf0)>>4)*10 + (time_buffer[4109] & 0x0f);  //y
				buffer[7] = ((time_buffer[4108] & 0xf0)>>4)*10 + (time_buffer[4108] & 0x0f);  //m
				buffer[9] = ((time_buffer[4107] & 0xf0)>>4)*10 + (time_buffer[4107] & 0x0f);  //d
				buffer[11] = ((time_buffer[4106] & 0xf0)>>4)*10 + (time_buffer[4106] & 0x0f);  //h
    		   		 buffer[13] = ((time_buffer[4105] & 0xf0)>>4)*10 + (time_buffer[4105] & 0x0f);  //m
				buffer[15] = ((time_buffer[4104] & 0xf0)>>4)*10 + (time_buffer[4104] & 0x0f);  //s

				buffer[30] = 0xfd;
				buffer[31] = 0xfd;

     	    			write(s, buffer, ACK_SIZE);
     	    			break;
			case  0x07:                   //choose--the data source
				if(buffer[4])
				{
					select_data_source(1);   //test
				}
				else
				{
					select_data_source(0);    //sys
				}
				buffer[0] = 0x7e;
				buffer[1] = 0x7e;

				buffer[2] = 0x00;
				buffer[3] = 0x27;

				buffer[28] = 0xcc;
				buffer[29] = 0xcc;

				buffer[30] = 0xfd;
				buffer[31] = 0xfd;

				write(s, buffer, ACK_SIZE);
				break;
			case  0x08:                   //statue
				fisk_threshold  = buffer[4];
				buffer[0] = 0x7e;
				buffer[1] = 0x7e;

				buffer[2] = 0x00;
				buffer[3] = 0x28;

				buffer[28] = 0xcc;
				buffer[29] = 0xcc;

				buffer[30] = 0xfd;
				buffer[31] = 0xfd;

				write(s, buffer, ACK_SIZE);
				break;
			case  0x09:                   //statue
				memcpy(recive_buffer, bram_vptr[1], BRAM_SIZE_RECIVE); //0x820001000
				lan_statue = connect_check();
				buffer[0]  = 0x7e;
				buffer[1]  = 0x7e;

				buffer[2]  = 0x00;
				buffer[3]  = 0x29;

				buffer[4] = recive_buffer[4110] ;          //channel
				buffer[5] = (lan_statue <<4) | (recive_buffer[4111] & 0x00ff);          //lan

				if ( lan_statue == 0xf)
				{
					buffer[6] = 0x3  ;              //iic
				}
				else
				{
					buffer[6] = 0  ;              //iic
				}
				buffer[7] = 0  ;

				buffer[8]  = recive_buffer[4096] ;        //fpga-v1005
				buffer[9]  = recive_buffer[4097] ;
				buffer[10] = recive_buffer[4098] ;
				buffer[11] = recive_buffer[4099] ;
				buffer[12] = recive_buffer[4100] ;
				buffer[13] = recive_buffer[4101] ;
				buffer[14] = recive_buffer[4102] ;
				buffer[15] = recive_buffer[4103] ;

				buffer[16] = 0x3 ;			//software version
				buffer[17] = 0x6 ;

				buffer[28] = 0xcc;
				buffer[29] = 0xcc;

				buffer[30] = 0xfd;
				buffer[31] = 0xfd;

				write(s, buffer, ACK_SIZE);
				break;
			case  0x10:                   //file  information
				memcpy(path, &buffer[18], MAX); //get the file name
				total = get_file_count(path);    //add: yanghx 2019.07.16  count the file
				buffer[0]  = 0x7e;
				buffer[1]  = 0x7e;

				buffer[2]  = 0x00;
				buffer[3]  = 0x30;

				buffer[4] =  (total>>8)&0xff;          //num
				buffer[5] =       total&0xff;          //num

				for(j = 0;j < 256 ; j++)
				{
					buffer[6+j] =  path[j];
				}
				buffer[262] = 0xcc;
				buffer[263] = 0xcc;

				buffer[264] = 0xfd;
				buffer[265] = 0xfd;

				write(s, buffer, 266);
				break;
			default:
				break;
		}
	}	
}
  /* no  use */
void process_conn_client(int s)
{
	ssize_t size = 0;
	char buffer[1024];
	
	for(;;)
	{
		size = read(0, buffer, 1024);
		if(size > 0)
		{
			write(s, buffer, size);
			size = read(s, buffer, 1024);
			write(1, buffer, size);
		}
	}	
}

int connect_check()
{
	int  net_fd;
	int  net_statue = 0;
	char statue[20],i;
	int  ret = 0;
	int  base_bx_state = 0;
	int  base_t_state = 0;
	char* lan[4] = {"/sys/class/net/eth0/operstate", "/sys/class/net/eth1/operstate","/sys/class/net/eth2/operstate","/sys/class/net/eth3/operstate"};
	for(i=0;i<2;i++)
	{
		net_fd =open(lan[i],O_RDONLY);
		if(net_fd < 0)
		{
			continue;
		}
		memset(statue,0,sizeof(statue));
		ret = read(net_fd,statue,10);

		if(NULL!=strstr(statue,"up"))
		{
			if(i==0)
			{
				base_bx_state = 1;
			}

			if(i==1)
			{
				base_t_state = 1;
			}
		}
		close(net_fd);
	}

	if (( base_bx_state == 1)&&( base_t_state == 1 ))
	{
		net_statue = 0xf;
	}
	else if ( base_t_state == 1 )
	{
		net_statue = 0x1;
	}

	return net_statue;
}
void set_datetime(int year, int mon, int mday, int hour, int  min, int sec)
{
	 time_t t ;
	 struct tm nowtime;
	 if( hour <= 8){
		 hour=hour+16;
		 mday= mday-1;
	 }else
	 {
		 hour=hour-8;
	 }
	 nowtime.tm_sec   = sec;
	 nowtime.tm_min   = min;
	 nowtime.tm_hour  = hour;
	 nowtime.tm_mday  = mday;
	 nowtime.tm_mon   = mon-1;
	 nowtime.tm_year  = year+100;
	 nowtime.tm_isdst = -1;
    t = mktime(&nowtime);
    stime(&t);
}
int get_file_count(char *root)
{
	DIR *dir;
	struct dirent * ptr;
	int total = 0;
	char path[MAX];
	char rootname[256] = {0};

	sprintf(rootname,"/smbshare/%s/",root);
	dir = opendir(rootname); /* 打开目录*/
	if(dir == NULL)
	{
		return 0xffff;
	}
	errno = 0;
	while((ptr = readdir(dir)) != NULL)  //编例
	{
		//顺序读取每一个目录项； 跳过“..”和“.”两个目录
		if(strcmp(ptr->d_name,".") == 0 || strcmp(ptr->d_name,"..") == 0)
		{
			continue;
		}
		//如果是目录，则递归调用 get_file_count函数  ：可以不使用
		if(ptr->d_type == DT_DIR)   //目录  向下遍历
		{
			sprintf(path,"%s/%s",root,ptr->d_name);
			total += get_file_count(path);
		}

		if(ptr->d_type == DT_REG)     //文件
		{
			total++;
		}
	}
	if(errno != 0)
	{
		printf("fail to read dir"); //失败则输出提示信息
		exit(1);
	}
	closedir(dir);
	return total;
}


