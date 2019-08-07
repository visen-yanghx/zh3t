/*
 * Gpio test
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "ubram.h"
#include "ugpio.h"

// The specific GPIO being used must be setup and replaced thru
// this code.  The GPIO of 240 is in the path of most the sys dirs
// and in the export write.
//
// Figuring out the exact GPIO was not totally obvious when there
// were multiple GPIOs in the system. One way to do is to go into
// the gpiochips in /sys/class/gpio and view the label as it should
// reflect the address of the GPIO in the system. The name of the
// the chip appears to be the 1st GPIO of the controller.
//
// The export causes the gpio240 dir to appear in /sys/class/gpio.
// Then the direction and value can be changed by writing to them.

// The performance of this is pretty good, using a nfs mount,
// running on open source linux, on the ML507 reference system,
// the GPIO can be toggled about every 4 usec.

// The following commands from the console setup the GPIO to be
// exported, set the direction of it to an output and write a 1
// to the GPIO.
//
// bash> echo 240 > /sys/class/gpio/export
// bash> echo out > /sys/class/gpio/gpio240/direction
// bash> echo 1 > /sys/class/gpio/gpio240/value

// if sysfs is not mounted on your system, the you need to mount it
// bash> mount -t sysfs sysfs /sys

// the following bash script to toggle the gpio is also handy for
// testing
//
// while [ 1 ]; do
//  echo 1 > /sys/class/gpio/gpio240/value
//  echo 0 > /sys/class/gpio/gpio240/value
// done

// to compile this, use the following command
// gcc gpio.c -o gpio

// The kernel needs the following configuration to make this work.
//
// CONFIG_GPIO_SYSFS=y
// CONFIG_SYSFS=y
// CONFIG_EXPERIMENTAL=y
// CONFIG_GPIO_XILINX=y




/*
 * GPIO has total 174 pins for ZynqMP
 * And first 78 pins are allocated to MIOs
 */
#define GPIO_1ST_PIN	306
#define MIO_1ST_PIN    	0
#define EMIO_1ST_PIN	78
const int start_pin = (GPIO_1ST_PIN + EMIO_1ST_PIN);		// gpiochip306 + 78 MIO  384?

int dma_mode = 0;

//static void gpio_set(int pin, unsigned char value);
static uint8_t gpio_get(int pin);


void gpio_init(int num)
{
	int exportfd;
	int directionfd;
	char gpio_directtion[128];
	int count = 0;
	char gpio_num[128];

	// The GPIO has to be exported to be able to see it
	// in sysfs
	for (count = 0; count < num; count++) {
		exportfd = open("/sys/class/gpio/export", O_WRONLY); // wr
		if (exportfd < 0)
		{
			printf("Cannot open GPIO to export it\n");
			exit(1);
		}

		sprintf(gpio_num, "%d", start_pin + count);
		write(exportfd, gpio_num, 4);
		close(exportfd);
	}
//why need ??only see gpio read?

	// set odd pin(PL->PS) direction of GPIO to be an input
	for (count = 1; count < num; count +=2 ) {
		sprintf(gpio_directtion, "/sys/class/gpio/gpio%d/direction", start_pin + count);
		directionfd = open(gpio_directtion, O_RDWR);
		if (directionfd < 0)
		{
			printf("Cannot open GPIO direction\n");
			exit(1);
		}

		write(directionfd, "in", 4);
		close(directionfd);
	}

	// set even pin(PS->PL) direction of GPIO to be an output
	for (count = 0; count < num; count +=2 ) {
		sprintf(gpio_directtion, "/sys/class/gpio/gpio%d/direction", start_pin + count);
		directionfd = open(gpio_directtion, O_RDWR);
		if (directionfd < 0)
		{
			printf("Cannot open GPIO direction\n");
			exit(1);
		}

		write(directionfd, "out", 4);
		close(directionfd);
	}

	printf("GPIO is prepared\n");

}

 void gpio_set(int pin, unsigned char value)
{
	int valuefd;
	char value_file[128];
	char value_str[10];

	memset(value_file, 0, sizeof(value_file));
	memset(value_str, 0, sizeof(value_str));

	// Get the GPIO value ready to be toggled
	sprintf(value_file, "/sys/class/gpio/gpio%d/value", start_pin + pin);
	valuefd = open(value_file, O_RDWR);
	if (valuefd < 0)
	{
		printf("Cannot open GPIO value\n");
		exit(1);
	}

	sprintf(value_str, "%d", value);
	write(valuefd, value_str, 2);
	close(valuefd);
}

static unsigned char gpio_get(int pin)
{
	int valuefd;
	char value_file[128];
	char value_str[10];
	unsigned char value;

	memset(value_file, 0, sizeof(value_file));
	memset(value_str, 0, sizeof(value_str));

	// Get the GPIO value ready to be toggled
	sprintf(value_file, "/sys/class/gpio/gpio%d/value", start_pin + pin);
	valuefd = open(value_file, O_RDONLY);
	if (valuefd < 0)
	{
		printf("Cannot open GPIO value\n");
		exit(1);
	}

	read(valuefd, value_str, 2);
	close(valuefd);
	value = atol(value_str);
	//printf("%s\n", value_str);
	return value;
}

void reset_pl()
{
#if 1
	gpio_set(26, 1);
	gpio_set(26, 1);
	gpio_set(26, 1);
	gpio_set(26, 1);
	gpio_set(26, 0);


     gpio_set(52, 0);//1
     gpio_set(54, 1);//7
     gpio_set(56, 1);//5
     gpio_set(58, 1);//6
     gpio_set(60, 1);//3  read_back
     gpio_set(62, 1);//4  warning
     gpio_set(64, 1);//2  record
#endif
}

void enable_data_source(uint8_t value)
{
	gpio_set(0, value);
   if(value){
	   bram_vptr[1][18]  = 1;
	   bram_vptr[1][19]  = 1;
		printf("Enable data source\n");
   }else{
	   bram_vptr[1][18]  = 0;
	   bram_vptr[1][19]  = 0;
		printf("waiting  data source\n");
   }

}

uint8_t flow_ctrl_req(int chan_num)
{
	int gpio = 13 + 2*chan_num;
	return gpio_get(gpio);
}

void flow_ctrl_ack(int chan_num)
{
	int gpio = 14 + 2*chan_num;
	gpio_set(gpio, 1);
	gpio_set(gpio, 0);
}

// data flow mode. 1 indicates simulation data.
void select_data_source(uint8_t mode)
{
	gpio_set(2, mode);
}

void check_en(uint8_t mode)
{
	gpio_set(48, mode);

}
void open_rtc(uint8_t mode)
{
	gpio_set(50, mode);

}
void slice_complete(uint8_t channel)
{
	uint8_t gpio;
	switch (channel)
	{
	case 0:
		gpio = 4;
		break;
	case 1:
		gpio = 6;
		break;
	case 2:
		gpio = 8;
		break;
	default:
		break;
	}

	gpio_set(gpio, 1);
	gpio_set(gpio, 0);
}

uint8_t slice_state(uint8_t channel)
{
	uint8_t ret = 0;
	switch (channel)
	{
	case 0:
		ret = gpio_get(3);
		break;
	case 1:
		ret = gpio_get(5);
		break;
	case 2:
		ret = gpio_get(7);
		break;
	default:
		break;
	}
	return ret;
}

void invalidate_bram()
{
	gpio_set(10, 1);
	gpio_set(10, 0);
//	printf("invalidate bram0 \n");
}

uint32_t get_bram()
{
	uint32_t bram_addr = 0;
	if (gpio_get(9)) {
		bram_addr = BRAM0_BA;
	}

	return bram_addr;
}

//////////////////////////////////
///// DMA
/////////////////////////////////
// dma work mode. 1 indicates using flag.
void use_dma_buf_flag(uint8_t mode)
{
	gpio_set(28, mode);
	dma_mode = mode;
}

uint8_t dma_buf_readable(int dma_num)
{
	int gpio = 19 + 2 * dma_num;
	if (dma_mode) {
		return gpio_get(gpio);
	}
	return 1;
}

void dma_buf_clear(int dma_num)
{
	int gpio = 20 + 2 * dma_num;
	if (dma_mode) {
		gpio_set(gpio, 1);
		gpio_set(gpio, 0);
	}
}

uint8_t dma_buf_writable(int dma_num)
{
	int gpio = 25 + 2 * dma_num;
	uint8_t ret = gpio_get(gpio);
	return ~ret;
}

// GPIO[34]\GPIO[32]\GPIO[30]
// |000	 128KB		|	100	  1MB								|
// |001    64B		|	101	 16MB								|
// |010   128B		|	110	 64B->128B->128KB->64B  每帧长自动变�?|
// |011    80B		|	111	 64B->80B->128KB->64B	每帧长自动变�?|
void select_frame_length(uint8_t value)
{
	gpio_set(30, value & 0x1);
	gpio_set(32, value & 0x2);
	gpio_set(34, value & 0x4);

//	 gpio_set(72, 0x1);//wf ad_data_mask = 1
//    gpio_set(74, 0x1);//wf pdw data_mask = 1
}

// GPIO[40]\GPIO[38]\GPIO[36]
// |000	  2048		|	100	   100	|
// |001   4096		|	101	  1000	|
// |010  32768		|	110	  2001	|
// |011     31		|	111	 65536  |
void select_frame_count(uint8_t value)
{
	gpio_set(36, value & 0x1);
	gpio_set(38, value & 0x2);
	gpio_set(40, value & 0x4);
}




