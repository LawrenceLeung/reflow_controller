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
#include <util/delay.h>
#include <stdint.h>

#include <string.h>
#include <stdio.h>

#include "usb_serial.h"

#include "oven_ssr.h"
#include "oven_timing.h"
#include "oven_pid.h"
#include "oven_profile.h"
#include "max6675.h"


// commands/config to controller from serial

volatile uint8_t mode_fake_out;
volatile uint8_t mode_fake_in;
volatile uint8_t mode_manual;

// TODO: there is a risk that some of these 16-bit values could be caught
// in a half-updated state, if the timer interrupt causes oven_update_4hz
// to be executed in the middle of process_message.
// Since these values are generally only used for testing, and not during
// actual reflow operations, this may not be a major concern.
volatile int16_t fake_temp_t;
volatile int16_t fake_temp_b;
volatile uint8_t manual_cmd_t;
volatile uint8_t manual_cmd_b;
volatile int16_t manual_target;



extern volatile uint8_t k_p;
extern volatile uint8_t k_i;
extern volatile uint8_t k_d;



#define CMD_RESET   1
#define CMD_GO      2
#define CMD_PAUSE   3
#define CMD_RESUME  4

// TODO: currently, only one comm_cmd can be processed per oven_update_4hz
// invocation, so multiple commands received within a ~0.25s window may be lost
volatile uint8_t comm_cmd;


// controller state

#define ST_FAULT    0
#define ST_IDLE     1
#define ST_RUN      2
#define ST_DONE     3
#define ST_PAUSE    4

const char *state_names[5] = { "fault","idle","run","done","pause" };

uint8_t state = ST_FAULT;

int16_t target;
uint16_t time;


char tx_msg[255];
volatile uint8_t tx_len = 0;

void fault(void)
{
// TODO: need to mask faults briefly at power-up, otherwise this will
// prematurely trip due to bogus thermocouple data
//    state = ST_FAULT;
//    ssr_fault();

    tx_len=sprintf_P(tx_msg,PSTR("FAULT\n"));
    if (!usb_configured()) return;
    usb_serial_write((void*)tx_msg,tx_len);
    tx_len = 0; // clear the length, so the control loop knows it can generate a new message
}

void thermocouple_fault(int16_t result)
{
    tx_len = sprintf_P(tx_msg,PSTR("TFAULT: %d\n"),
	            result);
    if (!usb_configured()) return;
     usb_serial_write((void*)tx_msg,tx_len);
     tx_len = 0; // clear the length, so the control loop knows it can generate a new message
}

void debugmsg(PGM_P  pmsg){
#ifdef DEBUG
	if (!usb_configured()) return;

    if (tx_len>0)
        usb_serial_write((void*)tx_msg,tx_len);
    tx_len = sprintf_P(tx_msg,pmsg);
    usb_serial_write((void*)tx_msg,tx_len);
#endif
    //tx_msg[0]=0;
   // tx_len = 0; // clear the length, so the control loop knows it can generate a new message
   //  _delay_ms(100);

}

void oven_output(uint8_t top, uint8_t bot)
{
    if( !mode_fake_out )
    {
        ssr_set(top,bot);
    }
    else
    {
        ssr_set(0,0);
    }
}

void oven_input(int16_t *top, int16_t *bot)
{
    if( !mode_fake_in )
    {
    	*top = max6675_read(0);

#ifndef BOTTOM_THERM
    	*bot = *top;
#else
        *bot = max6675_read(1);
#endif
    }
    else
    {
        *top = fake_temp_t;
        *bot = fake_temp_b;
    }
}

void oven_setup(void)
{
    state = ST_IDLE;

    mode_fake_out   = 0;
    mode_fake_in    = 0;
    mode_manual     = 0;

    manual_cmd_t    = 0;
    manual_cmd_b    = 0;
    manual_target   = 0;
    fake_temp_t     = 0;
    fake_temp_b     = 0;

    k_p   = DEFAULT_K_P;
    k_i   = DEFAULT_K_I;
    k_d   = DEFAULT_K_D;

    comm_cmd        = 0;

    target          = 0;
    time            = 0;
    tx_len          = 0;

    pid_reset();
    profile_reset();
    ssr_setup();
    max6675_setup();

    max6675_start();
    timing_setup();
}

void oven_update_120hz(void)
{
    ssr_update();
}


void oven_update_4hz(void)
{
    int16_t temp_t,temp_b;
    uint8_t cmd,cmd_t,cmd_b;
   
    oven_input(&temp_t,&temp_b);

    if(comm_cmd != 0)
    {
        switch(comm_cmd)
        {
            case CMD_RESET:
                profile_reset();
                pid_reset();
                manual_target   = 0;
                manual_cmd_t    = 0;
                manual_cmd_b    = 0;
                state           = ST_IDLE;
                break;
            case CMD_GO:
                if(state == ST_IDLE) {
                    state           = ST_RUN;
                }
                break;
            case CMD_PAUSE:
                if(state == ST_RUN) {
                    state           = ST_PAUSE;
                }
                break;
            case CMD_RESUME:
                if(state == ST_PAUSE) {
                    state           = ST_RUN;
                }
                break;
            default:
                fault();
        }

        comm_cmd = 0;
    }

    switch(state)
    {
        case ST_IDLE:
            target = manual_target;
            break;
        case ST_RUN:
            if(profile_update(&target))
                state = ST_DONE;
            break;
        case ST_PAUSE:
            // hold target
            break;
        case ST_DONE:
            target = 0;
            break;
        default:
            fault();
    }

    // manual target will always track target (so there aren't any surprises
    // when enabling manual mode)
    manual_target = target;
    
    cmd = pid_update(temp_t,target);

    if( state == ST_IDLE && mode_manual )
    {
        // full manual control from serial port
        cmd_t = manual_cmd_t;
        cmd_b = manual_cmd_b;
    }
    else
    {
        // manual power commands zeroed when not in full-manual mode (again: no surprises)
        manual_cmd_t = 0;
        manual_cmd_b = 0;

        // 25/75 top/bottom split (until bottom saturates)
        cmd_t = cmd >> 2;
        cmd_b = cmd - cmd_t;

        if(cmd_b >= 127) {
            cmd_t += (cmd_b - 127);
            cmd_b = 255;
        } else {
            cmd_b <<= 1;
        }

        if(cmd_t >= 127) {
            cmd_t = 255;
        } else {
            cmd_t <<= 1;
        }
    }

    oven_output(cmd_t,cmd_b);

    // produce status update message
    // (provided previous update has already been sent)
    if(!tx_len)
    {
        // expensive.. but we're only running at 4 Hz, so we have cycles to burn
        tx_len = sprintf_P(tx_msg,PSTR("%s,%u,%d,%d,%d,%u,%u,%u\n"),
            state_names[state],
            time,
            target,
            temp_t,
            temp_b,
            cmd,
            cmd_t,
            cmd_b);
    }

    time++;

}

char rx_msg[255];
uint8_t rx_cnt;

void process_message(const char *msg)
{
    // this is a ridiculously expensive function to invoke - a more efficient
    // command parser could be implemented, or a binary protocol established -
    // but, we're not expecting a lot of command traffic in this application,
    // and all of the timing-critical routines are handled by interrupts, so
    // there isn't a lot of downside to this expensive-but-easy implementation


    cli(); // temporarily disable interrupts to prevent any potential write errors

    if(sscanf_P(msg,PSTR("temp: %d, %d"),&fake_temp_t,&fake_temp_b) || \
       sscanf_P(msg,PSTR("cmd: %hhu, %hhu"),&manual_cmd_t,&manual_cmd_b) || \
       sscanf_P(msg,PSTR("target: %d"),&manual_target) || \
       sscanf_P(msg,PSTR("fake_out: %hhu"),&mode_fake_out) || \
       sscanf_P(msg,PSTR("fake_in: %hhu"),&mode_fake_in) || \
       sscanf_P(msg,PSTR("manual: %hhu"),&mode_manual) ||
       sscanf_P(msg,PSTR("pid: %d, %d, %d"),&k_p, &k_i, &k_d)) {
        ;
    } else if(strcmp_P(msg,PSTR("reset")) == 0) {
        comm_cmd = CMD_RESET;
    } else if(strcmp_P(msg,PSTR("go")) == 0) {
        comm_cmd = CMD_GO;
    } else if(strcmp_P(msg,PSTR("pause")) == 0) {
        comm_cmd = CMD_PAUSE;
    } else if(strcmp_P(msg,PSTR("resume")) == 0) {
        comm_cmd = CMD_RESUME;
    }

    sei();
}

// program entry point
int main(void)
{
    int16_t ret;
    char c;

    // no prescaler
    CLKPR = 0x80;
    CLKPR = 0;

    // hold CS high until init
    DDRB|=_BV(0);
    PORTB|=_BV(0);


    // initialize
    oven_setup();


    // sleep.  Makes the USB less cranky
    _delay_ms(1000);

    // wait for usb to initialize
    usb_init();
    while (!usb_configured());


    // wait an arbitrary bit for the host to complete its side of the init
    _delay_ms(1000);
    //timing_setup(); // timing setup last, since it enables timer interrupts (which invoke the update functions)

    // clear any stale packets
    usb_serial_flush_input();
    rx_cnt = 0;



    // run forever
    while(1)
    {
        // if the control loop has generated a status update message,
        // send it out over USB to the host
        if(tx_len)
        {
            usb_serial_write((void*)tx_msg,tx_len);
            tx_len = 0; // clear the length, so the control loop knows it can generate a new message
        }

        // receive individual characters from the host
        while( (ret = usb_serial_getchar()) != -1)
        {
            // all commands are terminated with a new-line
            c = ret;
            if(c == '\n') {
                // only process commands that haven't overflowed the buffer
                if(rx_cnt > 0 && rx_cnt < 255) {
                    rx_msg[rx_cnt] = '\0';
                    process_message(rx_msg);
                }
                rx_cnt = 0;
            } else {
                // buffer received characters
                rx_msg[rx_cnt] = c;
                if(rx_cnt != 255) rx_cnt++;
            }
        }
    }
}

