/*
 * zumo.c
 *
 *  Created on: Mar 1, 2020
 *      Author: jambox
 */

#include "zumo.h"
#include "uart.h"
#include <math.h>
#include "helpful.h"


char buffer[50];
char bleds[4] = {BLED0, BLED1, BLED2, BLED3};

static uint32_t line_calib[8];

void calibrate_line(int num_samps)
{

    uint32_t temp_adc_vals[8];
    int i, j;
    for (i = 0; i < num_samps; i++)
    {
        ReadIR(temp_adc_vals);
        for (j = 0; j < 6; j++)
        {
            line_calib[j] += temp_adc_vals[j];
        }
    }

    for (j = 0; j < 6; j++)
    {
        line_calib[j] = line_calib[j]/(float)num_samps;
    }

    sprintf(buffer, "calib: %u, %u, %u, %u, %u, %u\r\n", line_calib[5], line_calib[3], line_calib[1], line_calib[0], line_calib[2], line_calib[4]);
    WriteUART0(buffer);
//    while(1);
}

float lastValue = 0;
float read_line(uint32_t * vals)
{
    unsigned char on_line = 0;
    uint32_t avg = 0;
    uint32_t sum = 0;
    int i;

//    uint32_t ordered_vals[6] = {vals[5], vals[3], vals[1], vals[0], vals[2], vals[4]};
    uint32_t ordered_vals[4] = {vals[3], vals[1], vals[0], vals[2]};
    for (i = 0; i < 4; ++i)
    {
        int value = ordered_vals[i];
        if (value > 250) //200// I've now made 220 match w upper end of grey
        {
            on_line = 1;
        }

        if (value > 220) //150
        {
            avg += value * (i * 1000);
            sum += value;
        }
    }

    if (!on_line)
    {
        // If it last read to the left of center, return 0.
        if (lastValue < (4 -  1) * 1000/2.0)
        {
            return 0;
        }
        //otherwise return rightmost
        else
        {
            return (4 - 1);
        }
//        return lastValue/1000.0;
    }

    lastValue = avg/(float)sum;
    return lastValue/1000.0;
}

float error, e, prev_error, d_error;
float dt = 0.05;
int policy = 0;
//char do_once = 0;

void drive_line(float val, uint32_t * vals)
{

    ////////
    //NORMAL DRIVING
    ///////
    prev_error = error;
    error = 1.5 - val;
    e = (1.5 - val)/1.5;
    d_error = (error-prev_error)/dt;

//    sprintf(buffer, "error: %f\r\n", error);
//    WriteUART0(buffer);
    float speed_delim = 1 - fabs(error)/1.5;
    float rhs = speed_delim * MOTOR_ON + (e * MOTOR_ON/2.0) + MOTOR_ON/2.0;
    float lhs = speed_delim * MOTOR_ON - (e * MOTOR_ON/2.0) + MOTOR_ON/2.0;

//    sprintf(buffer, "rhs: %f, lhs: %f\r\n", rhs, lhs);
//    WriteUART0(buffer);

//    if (error < .3 && error > 0)
//    {
//        do_once = 0;
//    }


    setMotor(M2, 0, lhs);
    setMotor(M1, 0, rhs);

    //if we've lost the line

    uint8_t bias = get_actuation_flag();

    //normal lost line following
    if (fabs(error) == 1.5 || error == 0 && !bias) {
        if (error < 0){
            rotate(1);
            //WriteUART0("turning clockwise");
        }
        else if (error > 0){
            rotate(0);
            //WriteUART0("turning CCW");
        }
        else if (vals[0] == vals[2] && vals[1] == vals[3])
        {
            setMotor(M1, 0, MOTOR_OFF);
            setMotor(M2, 0, MOTOR_OFF);
        }

    }
    //biased lost line following
    else if (fabs(error) == 1.5 || error == 0 && bias)
    {
        uint8_t xc_state = get_xc_state();
        uint8_t policy= get_policy();
        uint8_t ret_policy = get_return_policy();

        uint8_t dir;
        if (!get_return_flag())
        {
            dir = xc_state & policy;
        }
        else
        {
            dir = xc_state & ret_policy;
        }

        rotate(dir);
    }


    return;
}



