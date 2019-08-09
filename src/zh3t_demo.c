
/*
      Created on: 8,7,2019
     Author: yanghx
     this is mybranchtest
zh3t_demo
write:
bram_vptr[1][0]  SH_time         ;  //0x82000000     bram_vptr[1][12]  TIME_sec    ;  //0x82000060
bram_vptr[1][1]  SH_mark         ;  //0x82000008	  bram_vptr[1][13]  TIME_min    ;  //0x82000068
bram_vptr[1][2]  DMA_LENGH       ;  //0x82000010	  bram_vptr[1][14]  TIME_hour   ;  //0x82000070
bram_vptr[1][3]  cut_dma0_ack    ;  //0x82000018	  bram_vptr[1][15]  TIME_day    ;  //0x82000078
bram_vptr[1][4]  cut_dma1_ack    ;  //0x82000020	  bram_vptr[1][16]  TIME_mon    ;  //0x82000080
bram_vptr[1][5]                  ;  //0x82000028	  bram_vptr[1][17]  TIME_year   ;  //0x82000088
bram_vptr[1][6]  read_sy_buf_ack ;  //0x82000030	  bram_vptr[1][18]  start_record;  //0x82000090
bram_vptr[1][7]                  ;  //0x82000038	  bram_vptr[1][19]  FILE_MARK   ;  //0x82000098
bram_vptr[1][8]                  ;  //0x82000040     bram_vptr[1][20]  speed       ;  //0x820000a0
                                                     bram_vptr[1][21]  speed_ls
bram_vptr[1][9]                  ;  //0x82000040     bram_vptr[1][22]  ls_close_mark  ;  //0x820000b0
                                                     bram_vptr[1][23]  mn_close_mark  ;  //0x820000b8
                                                     bram_vptr[1][24]  sy_close_mark  ;  //0x820000c0

                                                     bram_vptr[1][30] -[94] name      ;  //0x820000f0


read:
bram_vptr[1][515]  read_dma0_buf ;                   bram_vptr[1][518]  cut_dma0   ;
bram_vptr[1][516]  read_dma1_buf ;                   bram_vptr[1][519]  cut_dma1   ;
bram_vptr[1][521]  read_sy_buf   ;
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h>

#include "store.h"
#include "ubram.h"
#include "udma.h"
#include "udmabuf.h"

int main()
{
    printf("Start to processing:V3.9.1 2019-07-16-15:35 \n");

    start_heartbeat();

   /*INIT*/
    //////////////////////
   store_init();

   bram_vptr[1][2]  = 0x8000;   //DMA_lengh;
   bram_vptr[1][18] = 0 ;        //STAR_STORE = 0;
   bram_vptr[1][19] = 1 ;        //FILE_MARK = 1;

   bram_vptr[1][22] = 0 ;        //STOP_STORE = 0;
   bram_vptr[1][22] = 0 ;
   bram_vptr[1][23] = 0 ;


   bram_vptr[1][3]  = 0;
   bram_vptr[1][4]  = 0;
   bram_vptr[1][6]  = 0;        //read_sy_buf_ack = 0;

    /*WORK FLOW*/
    ///////////////////////
    store_prepare_recv();
    /*CONTROL*/
    //////////////////////
   store_control();

}

