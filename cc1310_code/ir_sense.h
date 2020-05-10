/*
 * ir_sense.h
 *
 *  Created on: May 10, 2020
 *      Author: jambox
 */

#ifndef IR_SENSE_H_
#define IR_SENSE_H_
#include "adc.h"
#include "gpio.h"
#include "helpful.h"
#include <string.h>
#include "uart.h"

#define NUM_SENSORS 4
#define NUM_SAMPLES 10

void IR_SenseSetup(void);
void ReadIR(uint32_t * vals);
void IR_SenseSetup(void);

#endif /* IR_SENSE_H_ */
