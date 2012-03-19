/**
 * Copyright (c) 2012, Lawrence Leung
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

#ifndef OVENCON_H
#define OVENCON_H


#ifdef __cplusplus
extern "C"{
#endif

#include <avr/io.h>
#include <util/delay.h>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>


extern void fault(void);
extern void thermocouple_fault(int16_t result);
extern void debugmsg(PGM_P  pmsg);
extern void oven_update_120hz(void);
extern void oven_update_4hz(void);

extern uint8_t is_usb_ready(void);



#ifdef __cplusplus
}
#endif



// Thermocouple settings

#define USE_THERMOCOUPLE

// Do we have 2 thermocouple chips?  If not, top=bottom
//#define BOTTOM_THERM

// Device level moving average
//#define TEMP_AVERAGING
//#define AVERAGE_BITS 2
//#define AVERAGE (1<<AVERAGE_BITS)

#ifndef BOTTOM_THERM
#define DEVICES 1
#else
#define DEVICES 2
#endif


//#define DEBUG


// default pid settings.  The term is actually 2^n for simplicity of calculation. These nubers should probably be <15

/// This is calibrated for a pizza oven
#define DEFAULT_K_P   13
#define DEFAULT_K_I   8
#define DEFAULT_K_D   2


// use the thermistor instead of the thermocouple?  
//#define USE_THERMISTOR

// use ADC 7
#define THERMISTOR_CHANNEL 7



// enable calibration profile as default

//#define CALIBRATION_PROFILE



#define READ(U, N) ((U) >> (N) & 1u)
#define SET(U, N) ((void)((U) |= 1u << (N)))
#define CLR(U, N) ((void)((U) &= ~(1u << (N))))
#define FLIP(U, N) ((void)((U) ^= 1u << (N)))


#endif

