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
void setMotor(int motor, int dir, int value)
{
    int DIR_pin, PWM_pin;
    if (motor == M1)
    {
       DIR_pin = M1_DIR;
       PWM_pin = M1_PWM;
    }
    else if (motor == M2)
    {
        DIR_pin = M2_DIR;
        PWM_pin = M2_PWM;
    }

    int set_val = 1022 - value;
    if (set_val < 0){
        set_val = 0;
    }

//    if (value < 0)
//    {
//        dir = !dir;
//    }

    GPIO_writeDio(DIR_pin, dir);
    PWMSet(PWM_pin, set_val);

}

void driver(uint32_t * vals)
{
    if((vals[0] > 500) && vals[1] > 500)
    {
        setMotor(M1, 1, MOTOR_OFF);
        setMotor(M2, 1, MOTOR_OFF);
    }
    else
    {
        setMotor(M1, 0, MOTOR_ON);
        setMotor(M2, 0, MOTOR_ON);
    }


}

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
char do_once = 0;

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

    if (error < .3 && error > 0)
    {
        do_once = 0;
    }


    setMotor(M2, 0, lhs);
    setMotor(M1, 0, rhs);

    if (fabs(error) == 1.5 || error == 0) {
        if (error < 0){
            setMotor(M1, 1, MOTOR_ON);
            setMotor(M2, 0, MOTOR_ON);
//            WriteUART0("turning clockwise");
        }
        else if (error > 0){
            setMotor(M1, 0, MOTOR_ON);
            setMotor(M2, 1, MOTOR_ON);
//            WriteUART0("turning CCW");
        }
        else if (vals[0] == vals[2] && vals[1] == vals[3])
        {
            setMotor(M1, 0, MOTOR_OFF);
            setMotor(M2, 0, MOTOR_OFF);
        }

    }


    ////////
    //CHECK FOR MESSAGES
    ///////
//    detect_poi(vals);

    return;
}

struct ColorTrack graphite = {.low_bound = GREY_LOW, .high_bound = GREY_HIGH, .idx = 0,
                               .curr_state = 0, .accum = 0, .stash_val = 0};

struct ColorTrack purp_left = {.low_bound = PURP_LOW, .high_bound = PURP_HIGH, .idx = 0,
                               .curr_state = 0, .accum = 0, .stash_val = 0};

struct ColorTrack purp_right = {.low_bound = PURP_LOW, .high_bound = PURP_HIGH, .idx = 0,
                               .curr_state = 0, .accum = 0, .stash_val = 0};

void detect_poi(uint32_t * vals)
{
    int i;
    purp_left.accum = 0;
    purp_left.stash_val = 0;

    purp_right.accum = 0;
    purp_right.stash_val = 0;

    graphite.accum = 0;
    graphite.stash_val = 0;

    for (i = 0; i < 4; i++)
    {
        //trying to binarize it here, if this doesnt work store all values as uint16_t, average over window
        //checking for #800080
        if (vals[(i + 2)] < purp_left.high_bound && vals[(i+2)] > purp_left.low_bound && (i % 2))
        {
            purp_left.accum += 1;
        }
        else if (vals[(i + 2)] < purp_right.high_bound && vals[(i+2)] > purp_right.low_bound && !(i % 2))
        {
            purp_right.accum += 1;
        }

        //checking for #333333
        //checks through RHS sensors
        if (vals[i+2] < graphite.high_bound && vals[i+2] > graphite.low_bound && !(i % 2))
        {
            graphite.accum +=1;
        }
    }

    if (graphite.accum >= 1) //<- changing this might be huuge
    {
        graphite.stash_val = 1;
    }
    else {
        graphite.stash_val = 0;
    }

    if (purp_left.accum >= 2)
    {
        purp_left.stash_val = 1;
    }
    else {
        purp_left.stash_val = 0;
    }

    if (purp_right.accum >= 2)
    {
        purp_right.stash_val = 1;
    }
    else {
        purp_right.stash_val = 0;
    }


    graphite.prev_vals[graphite.idx] = graphite.stash_val;
    purp_left.prev_vals[purp_left.idx] = purp_left.stash_val;
    purp_right.prev_vals[purp_right.idx] = purp_right.stash_val;

    graphite.prev_vals_ave = 0;
    purp_left.prev_vals_ave = 0;
    purp_right.prev_vals_ave = 0;
    for (i = 0; i < NUM_PREV_VALS; i++)
    {
        graphite.prev_vals_ave += graphite.prev_vals[i];

        purp_left.prev_vals_ave += purp_left.prev_vals[i];
        purp_right.prev_vals_ave += purp_right.prev_vals[i];
    }

//    sprintf(buffer, "ave: %u\r\n", purp.prev_vals_ave);
//    WriteUART0(buffer);

    if (graphite.prev_vals_ave > 5 && graphite.curr_state == 0)
    {
        GPIO_toggleDio(BLED0);
        graphite.curr_state = 1;
    }
    else
    {
        graphite.curr_state = 0;
    }

    if (purp_left.prev_vals_ave > 4 && state_track.xc_flags == 0b00)
    {
        state_track.xc_flags = 0b01;
    }
    else if (purp_right.prev_vals_ave > 4 && state_track.xc_flags == 0b01)
    {
        state_track.xc_flags = 0b10;
    }

    purp_left.idx = (purp_left.idx + 1) % NUM_PREV_VALS;
    purp_right.idx = (purp_right.idx + 1) % NUM_PREV_VALS;
    graphite.idx = (graphite.idx + 1) % NUM_PREV_VALS;

    return;
}
