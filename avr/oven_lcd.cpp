

#include "ovencon.h"
#include <util/delay.h>
#include <stdint.h>


#include "usb_serial.h"
#include "arduino/PCD8544.h"
#include "oven_lcd.h"
#include "arduino/core_pins.h"




extern int16_t temp_t, temp_b;


// Note pins are in arduino format
// Serial clock out (SCLK) - PF6
// Serial data out (DIN/MOSI) - PF5
// Data/Command select (D/C) - PF4
// LCD chip select (CS or SCE) - PF1
// LCD reset (RST) - PF0
// static initialization is gross but avr-g++ doesn't seem to do new/delete
PCD8544 nokia(PIN_F6,PIN_F5,PIN_F4,PIN_F1,PIN_F0);


// setup nokia
void lcd_init(){
    nokia.init();
}

// show USB powered message after usb connection is found
void lcd_usb_found_wait(){

    while (!usb_configured());
    nokia.clear();
    nokia.setCursor(0, 0);
    nokia.print("USB Host Detected");
    nokia.setCursor(0, 25);
    nokia.print("Waiting for host software");
    nokia.display();
}


// wait for the host
void lcd_host_dtr_wait(){
        // wait for DTR
    while (!(usb_serial_get_control() & USB_SERIAL_DTR));
    nokia.clear();
    nokia.setCursor(0, 0);
    nokia.print("USB Host Initialized");
    nokia.display();

}


// update in response to temp change
void lcd_update(){
    nokia.clear();
    nokia.setCursor(0, 0);
    nokia.print("Temp: ");
    nokia.print(temp_t>>2); // temp is in .25C
    nokia.print('.');
    uint8_t decimal=(temp_t & 0x03)*25;
    nokia.print(decimal);
    if (!decimal){
      nokia.print('0');
      nokia.print('0'); // 2 zeros
    }
    
    nokia.print("C");
    nokia.display();


}