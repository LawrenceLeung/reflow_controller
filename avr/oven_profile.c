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

#include"oven_profile.h"

typedef struct
{
    uint16_t    delta_time; // time steps to run at temp_rate (0.25s each)
    int16_t     temp_rate;  // rate of temperature change (1/1024 degrees per time step)
} s_profile_step;

#define STEPS 7

// profile table entries computed in reflow_profiles.ods spreadsheet
const s_profile_step profile[STEPS] = {
    { 960,   64 },
    { 720,  135 },
    { 120,  213 },
    { 180,   85 },
    { 120,    0 },
    { 180, -228 },
    { 360, -441 }
};

uint8_t     profile_step;   // current step
uint16_t    profile_time;   // time until next step
int32_t     profile_temp;   // current target temperature (in 1/1024 degree units)

void profile_reset(void)
{
    profile_step    = 0;
    profile_time    = profile[0].delta_time;
    profile_temp    = (25*1024); // room temp start point
}

uint8_t profile_update(int16_t *target)
{
    if(profile_step >= STEPS)
        return 1;

    profile_temp += profile[profile_step].temp_rate;

    if( --profile_time == 0 && ++profile_step < STEPS )
        profile_time = profile[profile_step].delta_time;

    *target = profile_temp >> 8;

    return 0;
}

