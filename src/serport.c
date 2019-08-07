
#include <unistd.h>
#include "serport.h"

static int serial_set(int fd,int speed, int flow_ctrl, int databits, int stopbits, int parity);
   
/******************************************************************* 
* function              serial_open
* port					port name
*******************************************************************/  
int serial_open(char* port)
{
	int fd = 0;
	fd = open( port, O_RDWR | O_NOCTTY | O_NDELAY);
	if (-1 == fd)
	{
		perror("Can't Open Serial Port");
	}
	return fd;
}

int serial_init(int fd)
{	
	int ret = 0;
	
	ret = serial_set(fd, 115200, 0, 8, 1, 'N');
	if(ret == -1)
	{
		printf("serial set failed\n");
	}
	
	return ret;
}

/******************************************************************* 
* function:               serial_set
*
*******************************************************************/  
static int serial_set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
	int   i;  
	int   speed_arr[] = { B115200, B19200, B9600, B4800, B2400, B1200, B300};  
	int   name_arr[] = {115200,  19200,  9600,  4800,  2400,  1200,  300};  
	       
	struct termios options;  
	if (tcgetattr(fd, &options) !=  0)
	{  
		perror("tcgetattr failed\n");
		return -1;
	}  

	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)  
	{  
		if  (speed == name_arr[i])  
		{               
			cfsetispeed(&options, speed_arr[i]);   
			cfsetospeed(&options, speed_arr[i]);    
		}  
	}       
	 
	// set flag
	options.c_cflag |= CLOCAL;  
	options.c_cflag |= CREAD;  

	switch(flow_ctrl) {
	case 0 :
		  options.c_cflag &= ~CRTSCTS;
		  break;
	case 1 :
		  options.c_cflag |= CRTSCTS;
		  break;
	case 2 :
		  options.c_cflag |= IXON | IXOFF | IXANY;
		  break;
	}  
	options.c_cflag &= ~CSIZE;  
	switch (databits) {
	case 5:
		 options.c_cflag |= CS5;
		 break;
	case 6:
		 options.c_cflag |= CS6;
		 break;
	case 7:
		 options.c_cflag |= CS7;
		 break;
	case 8:
		 options.c_cflag |= CS8;
		 break;
	default:
		 fprintf(stderr, "Unsupported data size\n");
		 return -1;
	}  

	switch (parity) {
	case 'n':
	case 'N':
		 options.c_cflag &= ~PARENB;
		 options.c_iflag &= ~INPCK;
		 options.c_iflag &= ~ICRNL;
		 options.c_iflag &= ~IXON;
		 break;
	case 'o':
	case 'O':
		 options.c_cflag |= (PARODD | PARENB);
		 options.c_iflag |= INPCK;
		 break;
	case 'e':
	case 'E':
		 options.c_cflag |= PARENB;
		 options.c_cflag &= ~PARODD;
		 options.c_iflag |= INPCK;
		 break;
	case 's':
	case 'S':
		 options.c_cflag &= ~PARENB;
		 options.c_cflag &= ~CSTOPB;
		 break;
	default:
		 fprintf(stderr, "Unsupported parity\n");
		 return -1;
	}   

	switch (stopbits) {
	case 1:
		options.c_cflag &= ~CSTOPB; break;
	case 2:
		options.c_cflag |= CSTOPB; break;
	default:
		fprintf(stderr, "Unsupported stop bits\n");
		return -1;
	}  
	options.c_oflag &= ~OPOST;  
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  
	//options.c_lflag &= ~(ISIG | ICANON);  
	 
	options.c_cc[VTIME] = 10;
	options.c_cc[VMIN] = 1;
	options.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;

	tcflush(fd,TCIFLUSH);

	if (tcsetattr(fd, TCSANOW, &options) != 0)
	{  
		perror("com set error!\n");    
		return -1;
	}  
	return 0;
}

/******************************************************************* 
*
*******************************************************************/  
int serial_recv(int fd, char *rcv_buf, int data_len)
{
	int len,fs_sel;  
	fd_set fs_read;  
	 
	struct timeval time;  
	FD_ZERO(&fs_read);  
	FD_SET(fd, &fs_read);
	 
	time.tv_sec = 3;  
	time.tv_usec = 0;  
	 
	fs_sel = select(fd+1, &fs_read, NULL,NULL, &time);
	if(fs_sel)  
	{  
		len = read(fd, rcv_buf, data_len);
		if (len < data_len)
			printf("uart recv failed\n");
		else {
			printf("uart recv: %s\n", rcv_buf);
		}
	}  
	return len;
}

/******************************************************************** 
*
*******************************************************************/  
int serial_send(int fd, char *send_buf, int data_len)
{
	int len = 0;  
	len = write(fd, send_buf, data_len);
	if (len < data_len)
		printf("uart send failed\n");
	return len;
}

/******************************************************************* 
*
*******************************************************************/  
int serial_close(int fd)
{
	close(fd);
	return 0;
}
