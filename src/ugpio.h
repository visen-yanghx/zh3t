/*
 * libgpio.h
 *
 *  Created on: May 9, 2018
 *      Author: terence
 */

#ifndef SRC_UGPIO_H_
#define SRC_UGPIO_H_


void gpio_init(int num);
/*
 * Even number GPIO used for PS to PL
 * and odd number GPIO used for PL to PS
 */
void select_data_source(uint8_t mode);
void check_en(uint8_t mode);
void open_rtc(uint8_t mode);
extern void enable_data_source(uint8_t value);
void slice_complete(uint8_t channel);
uint8_t slice_state(uint8_t channel);
uint32_t get_bram();
void invalidate_bram();
uint8_t flow_ctrl_req(int chan_num);
void flow_ctrl_ack(int chan_num);
void reset_pl();

void use_dma_buf_flag(uint8_t value);
uint8_t dma_buf_readable(int dma_num);
uint8_t dma_buf_writable(int dma_num);
void dma_buf_clear(int dma_num);

void select_frame_count(uint8_t value);
void select_frame_length(uint8_t value);

#endif /* SRC_UGPIO_H_ */
