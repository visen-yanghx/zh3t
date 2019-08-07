#ifndef __C_SERIAL_PORT_H
#define __C_SERIAL_PORT_H

#include<stdio.h>      							/*��׼�����������*/  
#include<stdlib.h>     							/*��׼�����ⶨ��*/  
#include<unistd.h>     							/*Unix ��׼��������*/  
#include<sys/types.h>   
#include<sys/stat.h>     
#include<fcntl.h>      							/*�ļ����ƶ���*/  
#include<termios.h>    							/*PPSIX �ն˿��ƶ���*/  
#include<errno.h>     							/*����Ŷ���*/  
#include<string.h>  


typedef struct{
        char    prompt;         						//prompt after reciving data
        int     baudrate;               					//baudrate
        char    databit;                					//data bits, 5, 6, 7, 8
        char    debug;          						//debug mode, 0: none, 1: debug
        char    echo;                   					//echo mode, 0: none, 1: echo
        char    fctl;                  					 //flow control, 0: none, 1: hardware, 2: software
        char    tty;                    					//tty: 0, 1, 2, 3, 4, 5, 6, 7
        char    parity;        						 //parity 0: none, 1: odd, 2: even
        char    stopbit;               					 //stop bits, 1, 2
        const int reserved;     					//reserved, must be zero
}portinfo_t;
typedef portinfo_t *pportinfo_t;


int serial_open(char* port);
int serial_init(int fd);
int serial_recv(int fd, char *rcv_buf,int data_len);
int serial_send(int fd, char *send_buf,int data_len);
int serial_close(int fd);

#endif

