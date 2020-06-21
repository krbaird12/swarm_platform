/*
 * zumo_moves.h
 *
 *  Created on: Jun 21, 2020
 *      Author: jambox
 */

#ifndef ZUMO_MOVES_H_
#define ZUMO_MOVES_H_

#include <stdint.h>
#include "zumo.h"
#include "uart.h"
#include "helpful.h"

void openloop_turn(uint8_t dir);
void begin_openloop(void);
void end_openloop(void);
void set_total_count(uint16_t tot_count);
#endif /* ZUMO_MOVES_H_ */