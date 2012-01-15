/**
 * Copyright (c) 2011, Daniel Strother < http://danstrother.com/ >
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   - The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "oven_pid.h"


volatile uint8_t k_p;
volatile uint8_t k_i;
volatile uint8_t k_d;

const uint8_t k_div = 8;
#define k_delay 40

// state
int16_t pid_prev[k_delay]; // previous temperatures (0.25C units)
int32_t pid_int; // integral

uint8_t pid_prev_index;

int16_t pid_prev_update(int16_t prev)
{
    int16_t popped = pid_prev[pid_prev_index];
    pid_prev[pid_prev_index] = prev;

    pid_prev_index++;
    if(pid_prev_index >= k_delay)
        pid_prev_index = 0;

    return popped;
}

void pid_reset(void)
{
    uint8_t i;
    
    for(i=0;i<k_delay;i++)
        pid_prev[i] = 100; // room temp
    
    pid_int = 0;
    pid_prev_index = 0;
}

// input is current temperature and target temperature (in 0.25C units)
// returned command is 0-255 (0 is off; 255 is full power)
// PID algorithm based on information presented in Tim Wescott's "PID wihout a PhD" article
uint8_t pid_update(int16_t temp, int16_t target)
{
    int16_t error, derivative;
    int32_t command;

    // calculate terms
    error       = target - temp; // error term must be positive when we're ramping up
    derivative  = pid_prev_update(temp) - temp; // derivative term must be negative when we're ramping up

    // TODO: consider using derivative of error, rather than temp

    // sum weighted terms
    command     = ((int32_t)error)      << k_p;
    command    += ((int32_t)pid_int)    << k_i;
    command    += ((int32_t)derivative) << k_d;

    // post-divide
    command   >>= k_div;

    // only update integral if output is not saturated (or if change would reduce saturation)
    if( (command >= 0 && command <= 255) || (command > 0 && error < 0) || (command < 0 && error > 0) )
        pid_int     += error;

    // limit command
    if(command < 0)
        command     = 0;
    else if(command > 255)
        command     = 255;

    return (uint8_t)command;
}

