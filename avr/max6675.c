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

#include <avr/io.h>
#include <util/delay.h>
#include "max6675.h"

extern void fault(void);

#define AVERAGE_BITS 2
#define AVERAGE (1<<AVERAGE_BITS)
#define DEVICES 2

int16_t temps[DEVICES][AVERAGE];

static void _max6675_select(uint8_t device, uint8_t cs)
{
    switch(device)
    {
        // PB0
        case 0:
            if(cs)  PORTB &= ~(_BV(0)); // active-low
            else    PORTB |= _BV(0);
            DDRB |= _BV(0);
            break;
        // PB4
        case 1:
            if(cs)  PORTB &= ~(_BV(4)); // active-low
            else    PORTB |= _BV(4);
            DDRB |= _BV(4);
            break;
    }
}

void max6675_setup(void)
{
    uint8_t i,j;

    // enable SPI; polled; MSB-first; Master; idle: low; sample: div: 16 (1 MHz clock)
    SPCR    = _BV(SPE) | _BV(MSTR) | _BV(SPR0);

    // set chip-selects inactive and initialize averages
    for(i=0;i<DEVICES;++i)
    {
        _max6675_select(i,0);
        for(j=0;j<AVERAGE;++j)
            temps[i][j] = 100; // 25C
    }

    // set SPI pin directions
    DDRB    |= _BV(1) | _BV(2); // SCLK and MOSI as outputs
    DDRB    &= ~(_BV(3));       // MISO as input

    // start initial conversion
    for(i=0;i<DEVICES;++i)
    {
        _delay_us(100);
        _max6675_select(i,1);
        _delay_us(100);
        _max6675_select(i,0);
    }
}

int16_t max6675_read(uint8_t device)
{ 
    int16_t result;
    int16_t avg;
    uint8_t i;

    // select device
    _max6675_select(device,1);

    // read MSbyte
    SPDR = 0xFF;
    loop_until_bit_is_set(SPSR,SPIF);
    result = SPDR;
    result <<= 8;

    // read LSbyte
    SPDR = 0xFF;
    loop_until_bit_is_set(SPSR,SPIF);
    result |= SPDR;
    
    // de-select device (starts new conversion)
    _max6675_select(device,0);

    // check for open/shorted line or open-thermocouple flag
    if( result == 0x0000 || result == 0xFFFF || result & (1<<2) ) {
        result = 0xFFFF;
        fault();
    }

    // result is in upper 13 bits (12 real bits; MSbit is dummy sign (always 0))
    result >>= 3;

    // check range (5-300 degrees)
    if( result < 10 || result > 1200) {
        result = 0x0FFF;
        fault();
    }

    // average
    avg = result;
    for(i=1;i<AVERAGE;++i)
    {
        temps[device][i] = temps[device][i-1];
        avg += temps[device][i];
    }
    temps[device][0] = result;

    avg >>= AVERAGE_BITS;

    return avg;
}

