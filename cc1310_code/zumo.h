/*
 * zumo.h
 *
 *  Created on: Mar 1, 2020
 *      Author: jambox
 */

#ifndef ZUMO_H_
#define ZUMO_H_

#include "gpio.h"
//#include "pwm.h"
#include "ir_sense.h"
#include "color_track.h"
#include "state_track.h"
#include "zumo_moves.h"
//TODO: serious organization decisions need to be made about where these pinds are going to be defined
//      as of rn zumo is a high level abstraction and should not have low level pin decs

#define M1 1
#define M2 2
#define M2_PWM IOID_12//pin 9 on zumo
#define M2_DIR IOID_19//pin 7 on zumo  //THIS SWITCHES TO IOID_19 for future revisions!!!
#define M1_PWM IOID_11//pin 10 on zumo
#define M1_DIR IOID_20//pin 8 on zumo

//#define MR_Sense IOID_25
//#define IR_Sense IOID_23
//#define IL_Sense IOID_24
//#define ML_Sense IOID_26
//#define ER_Sense IOID_27
//#define EL_Sense IOID_28

//WILL HAVE TO ENABLE THIS BEFORE USE in gpio.c
#define LED_Sense IOID_18 //connected to pin 2 on zum (with jumper on IR breakout set there) //

#define MOTOR_ON 256
#define MOTOR_OFF 0
#define MOTOR_TURN 64//64

float read_line(uint32_t * vals);
void drive_line(float val, uint32_t * vals);
void calibrate_line(int num_samps);

void execute_policy(void);
#endif /* ZUMO_H_ */
