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


#include "ovencon.h"
#include <avr/interrupt.h>
#include <stdint.h>

#include "oven_timing.h"


volatile static uint8_t div;

void timing_setup(void)
{
    div     = 0;

    cli(); // turn off interrupts temporarily

    DDRB |= _BV(5) | _BV(6); // enable PWM (maybe this will make my timer work?)

    // CTC with ICRn TOP, clk/8
//    ICR1    = 16807; // 16MHz/(8*16807) = 119Hz (slightly under mains frequency; will sync with external interrupt)

    TCNT1 = 0;
    TCCR1A  = 0;
    TCCR1B  = _BV(WGM13) | _BV(WGM12) |  _BV(CS11); // CTC, clk div 8
    TCCR1C  = 0;

    // 119.998 Hz (no external sync)
#if (F_CPU==16000000)
    ICR1    = 16667; 
    OCR1A   = 14000; 
#elif (F_CPU==8000000)
    ICR1    = 8404; 
    OCR1A   = 7000;
#else
#error "FCPU not 16mhz or 8mhz"
#endif


    TIMSK1  = _BV(OCIE1A); // enable OCRA1 interrupt


    // enable external interrupt (INT0/PD0 and INT1/PD1; falling edge)
    // TODO: not currently used; requires extra hardware to sample mains
    // (e.g. a low-voltage transformer)
//    DDRD    &= ~(_BV(0) | _BV(1));
//    PORTD   |= _BV(0) | _BV(1); // pull-up
//    EICRA   = _BV(ISC01) | _BV(ISC11);
//    EIMSK   = _BV(INT0) | _BV(INT1);


    // E6 blinky
    DDRE |= _BV(6);
    PORTE |= (_BV(6));

    sei(); //reenable interrupts
}

// timer interrupt
ISR(TIMER1_COMPA_vect)
{
    oven_update_120hz();
    div++;

    // re-enable interrupts
    sei();

    // execute control update every 30 steps (120/30 = 4 Hz)
    if (30==div)
    {
        div = 0;
        oven_update_4hz();
    	PORTE ^=_BV(6); //blink E6
    }
}

// AC zero-cross interrupts - just clear timer
//ISR(INT0_vect) { TCNT1 = 0; }
//ISR(INT1_vect) { TCNT1 = 0; }


