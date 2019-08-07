/*
 * store.h
 *
 *  Created on: Jun 8, 2018
 *      Author: terence
 */

#ifndef SRC_STORE_H_
#define SRC_STORE_H_


#include <stdint.h>


#define MAX_DIR_NAME_LENGTH 256

//extern char recordDirName[MAX_DIR_NAME_LENGTH];

int store_init();
int read_time();
void store_control();
int store_prepare_recv();
#endif /* SRC_STORE_H_ */
