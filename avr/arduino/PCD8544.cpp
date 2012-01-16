/*
$Id:$

PCD8544 LCD library!

Copyright (C) 2010 Limor Fried, Adafruit Industries

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

//#include <Wire.h>
#include <avr/pgmspace.h>
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include <util/delay.h>
#include <stdlib.h>

#include "PCD8544.h"
#include "glcdfont.cpp"

uint8_t is_reversed = 0;


// a 5x7 font table
extern uint8_t PROGMEM font[];

// the memory buffer for the LCD (preloaded with splashscreen logo)
uint8_t pcd8544_buffer[LCDWIDTH * LCDHEIGHT / 8] = {
		 /* page 0 (lines 0-7) */
		  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x80,0x80,0xc0,0x60,0x30,0x18,0xc,0xfc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xc0,0x40,0x40,0x40,0x80,0x0,0x0,
		  0x0,0x0,0x0,0x0,0x0,0x0,0x80,0x40,0x40,0x0,0xc0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,
		  /* page 1 (lines 8-15) */
		  0x0,0x0,0x0,0x0,0x0,0xc0,0xf0,0x70,0xd0,0x98,0x8,0xc,0x4,0x6,0x2,0x3,
		  0x1,0x0,0x0,0x0,0x0,0x0,0x0,0xff,0xff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x7f,0x4,0x4,0x1c,0x23,0x40,0x3c,
		  0x4a,0x4a,0x4a,0x2c,0x0,0x2,0x7f,0x2,0x2,0x0,0x3f,0x40,0x0,0x3c,0x42,0x42,
		  0x42,0x3c,0x0,0x6,0x38,0x40,0x38,0x6,0x38,0x40,0x38,0x6,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,
		  /* page 2 (lines 16-23) */
		  0x0,0x0,0x0,0x0,0xf0,0x3f,0x1,0x0,0x4,0xe,0xb,0x18,0x10,0x18,0xc,0x8,
		  0x90,0xe0,0x30,0x10,0xd8,0x8,0x8,0xf,0x57,0xc,0x4,0xc,0xac,0x2c,0x18,0x18,
		  0x30,0x60,0xc0,0x80,0x0,0x0,0x0,0x0,0x0,0xc0,0x20,0x10,0x10,0x10,0x20,0x0,
		  0x0,0x0,0x80,0x80,0x80,0x0,0x0,0x0,0x80,0x80,0x80,0x80,0x0,0x0,0x80,0xe0,
		  0x80,0x80,0x0,0x0,0x80,0x0,0x80,0x80,0x0,0x0,0x80,0x80,0x80,0x0,0x0,0x0,
		  0xf0,0x0,0x0,0x0,
		  /* page 3 (lines 24-31) */
		  0x0,0x0,0x18,0x7f,0xd1,0x10,0x1c,0x4,0x4,0x1c,0x10,0x18,0x8,0x8,0x8,0xfe,
		  0x81,0x84,0x44,0x44,0x78,0xdd,0xb6,0x12,0x1a,0x93,0xe6,0x6,0x99,0x70,0x0,0x18,
		  0x0,0x4,0x4,0x3,0xff,0xf8,0x0,0x0,0x0,0x7,0x8,0x10,0x10,0x10,0x8,0x0,
		  0x0,0xf,0x10,0x10,0x10,0xf,0x0,0x0,0x1f,0x0,0x0,0x0,0x1f,0x0,0x0,0xf,
		  0x10,0x10,0x0,0x0,0x1f,0x1,0x0,0x0,0x0,0xf,0x10,0x10,0x10,0xf,0x0,0x0,
		  0xf,0x10,0x0,0x0,
		  /* page 4 (lines 32-39) */
		  0x0,0x0,0x0,0x0,0x1,0xf,0x3c,0xe0,0x80,0x80,0xc0,0x80,0x90,0x78,0x28,0x4,
		  0x3,0x2,0x6,0xc,0x8,0xfc,0x91,0x91,0x11,0x18,0x18,0x65,0x23,0x41,0x81,0x6,
		  0x8,0x88,0xcc,0x68,0x3f,0x1f,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,
		  /* page 5 (lines 40-47) */
		  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x3,0x2,0x6,0x4,0xc,0x8,0x18,0x10,
		  0x30,0x20,0x60,0x40,0xfc,0x45,0x64,0x27,0x30,0x30,0x10,0x10,0x10,0x19,0x1c,0x1c,
		  0xe,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
		  0x0,0x0,0x0,0x0
};


// reduces how much is refreshed, which speeds it up!
// originally derived from Steve Evans/JCW's mod but cleaned up and
// optimized
//#define enablePartialUpdate

#ifdef enablePartialUpdate
static uint8_t xUpdateMin, xUpdateMax, yUpdateMin, yUpdateMax;
#endif



static void updateBoundingBox(uint8_t xmin, uint8_t ymin, uint8_t xmax, uint8_t ymax) {
#ifdef enablePartialUpdate
  if (xmin < xUpdateMin) xUpdateMin = xmin;
  if (xmax > xUpdateMax) xUpdateMax = xmax;
  if (ymin < yUpdateMin) yUpdateMin = ymin;
  if (ymax > yUpdateMax) yUpdateMax = ymax;
#endif
}

PCD8544::PCD8544(int8_t SCLK, int8_t DIN, int8_t DC, int8_t CS, int8_t RST) {
  _din = DIN;
  _sclk = SCLK;
  _dc = DC;
  _rst = RST;
  _cs = CS;
  cursor_x = cursor_y = 0;
  textsize = 1;
  textcolor = BLACK;
}



PCD8544::PCD8544(int8_t SCLK, int8_t DIN, int8_t DC, int8_t RST) {
  _din = DIN;
  _sclk = SCLK;
  _dc = DC;
  _rst = RST;
  _cs = -1;
  cursor_x = cursor_y = 0;
  textsize = 1;
  textcolor = BLACK;

}

void PCD8544::drawbitmap(uint8_t x, uint8_t y, 
			const uint8_t *bitmap, uint8_t w, uint8_t h,
			uint8_t color) {
  for (uint8_t j=0; j<h; j++) {
    for (uint8_t i=0; i<w; i++ ) {
      if (pgm_read_byte(bitmap + i + (j/8)*w) & _BV(j%8)) {
	my_setpixel(x+i, y+j, color);
      }
    }
  }

  updateBoundingBox(x, y, x+w, y+h);
}


void PCD8544::drawstring(uint8_t x, uint8_t y, char *c) {
  cursor_x = x;
  cursor_y = y;
  print(c);
}


void PCD8544::drawstring_P(uint8_t x, uint8_t y, const char *str) {
  cursor_x = x;
  cursor_y = y;
  while (1) {
    char c = pgm_read_byte(str++);
    if (! c)
      return;
    print(c);
  }
}

void  PCD8544::drawchar(uint8_t x, uint8_t y, char c) {
  if (y >= LCDHEIGHT) return;
  if ((x+5) >= LCDWIDTH) return;

  for (uint8_t i =0; i<5; i++ ) {
    uint8_t d = pgm_read_byte(font+(c*5)+i);
    for (uint8_t j = 0; j<8; j++) {
      if (d & _BV(j)) {
	my_setpixel(x+i, y+j, textcolor);
      }
      else {
      my_setpixel(x+i, y+j, !textcolor);
      }
    }
  }
  for (uint8_t j = 0; j<8; j++) {
    my_setpixel(x+5, y+j, !textcolor);
  }
  updateBoundingBox(x, y, x+5, y + 8);
}

#if defined(ARDUINO) && ARDUINO >= 100
  size_t PCD8544::write(uint8_t c) {
#else
  void PCD8544::write(uint8_t c) {
#endif
  if (c == '\n') {
    cursor_y += textsize*8;
    cursor_x = 0;
  } else if (c == '\r') {
    // skip em
  } else {
    drawchar(cursor_x, cursor_y, c);
    cursor_x += textsize*6;
    if (cursor_x >= (LCDWIDTH-5)) {
      cursor_x = 0;
      cursor_y+=8;
    }
    if (cursor_y >= LCDHEIGHT) 
      cursor_y = 0;
  }
#if ARDUINO >= 100
  return 1;
#endif
}

void PCD8544::setCursor(uint8_t x, uint8_t y){
  cursor_x = x; 
  cursor_y = y;
}


// bresenham's algorithm - thx wikpedia
void PCD8544::drawline(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, 
		      uint8_t color) {
  uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  // much faster to put the test here, since we've already sorted the points
  updateBoundingBox(x0, y0, x1, y1);

  uint8_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int8_t err = dx / 2;
  int8_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;}

  for (; x0<=x1; x0++) {
    if (steep) {
      my_setpixel(y0, x0, color);
    } else {
      my_setpixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}


// filled rectangle
void PCD8544::fillrect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, 
		      uint8_t color) {

  // stupidest version - just pixels - but fast with internal buffer!
  for (uint8_t i=x; i<x+w; i++) {
    for (uint8_t j=y; j<y+h; j++) {
      my_setpixel(i, j, color);
    }
  }

  updateBoundingBox(x, y, x+w, y+h);
}

// draw a rectangle
void PCD8544::drawrect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, 
		      uint8_t color) {
  // stupidest version - just pixels - but fast with internal buffer!
  for (uint8_t i=x; i<x+w; i++) {
    my_setpixel(i, y, color);
    my_setpixel(i, y+h-1, color);
  }
  for (uint8_t i=y; i<y+h; i++) {
    my_setpixel(x, i, color);
    my_setpixel(x+w-1, i, color);
  } 

  updateBoundingBox(x, y, x+w, y+h);
}

// draw a circle outline
void PCD8544::drawcircle(uint8_t x0, uint8_t y0, uint8_t r, 
			uint8_t color) {
  updateBoundingBox(x0-r, y0-r, x0+r, y0+r);

  int8_t f = 1 - r;
  int8_t ddF_x = 1;
  int8_t ddF_y = -2 * r;
  int8_t x = 0;
  int8_t y = r;

  my_setpixel(x0, y0+r, color);
  my_setpixel(x0, y0-r, color);
  my_setpixel(x0+r, y0, color);
  my_setpixel(x0-r, y0, color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    my_setpixel(x0 + x, y0 + y, color);
    my_setpixel(x0 - x, y0 + y, color);
    my_setpixel(x0 + x, y0 - y, color);
    my_setpixel(x0 - x, y0 - y, color);
    
    my_setpixel(x0 + y, y0 + x, color);
    my_setpixel(x0 - y, y0 + x, color);
    my_setpixel(x0 + y, y0 - x, color);
    my_setpixel(x0 - y, y0 - x, color);
    
  }
}

void PCD8544::fillcircle(uint8_t x0, uint8_t y0, uint8_t r, 
			uint8_t color) {
  updateBoundingBox(x0-r, y0-r, x0+r, y0+r);

  int8_t f = 1 - r;
  int8_t ddF_x = 1;
  int8_t ddF_y = -2 * r;
  int8_t x = 0;
  int8_t y = r;

  for (uint8_t i=y0-r; i<=y0+r; i++) {
    my_setpixel(x0, i, color);
  }

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    for (uint8_t i=y0-y; i<=y0+y; i++) {
      my_setpixel(x0+x, i, color);
      my_setpixel(x0-x, i, color);
    } 
    for (uint8_t i=y0-x; i<=y0+x; i++) {
      my_setpixel(x0+y, i, color);
      my_setpixel(x0-y, i, color);
    }    
  }
}


void PCD8544::my_setpixel(uint8_t x, uint8_t y, uint8_t color) {
  if ((x >= LCDWIDTH) || (y >= LCDHEIGHT))
    return;

  // x is which column
  if (color) 
    pcd8544_buffer[x+ (y/8)*LCDWIDTH] |= _BV(y%8);  
  else
    pcd8544_buffer[x+ (y/8)*LCDWIDTH] &= ~_BV(y%8); 
}



// the most basic function, set a single pixel
void PCD8544::setPixel(uint8_t x, uint8_t y, uint8_t color) {
  if ((x >= LCDWIDTH) || (y >= LCDHEIGHT))
    return;

  // x is which column
  if (color) 
    pcd8544_buffer[x+ (y/8)*LCDWIDTH] |= _BV(y%8);  
  else
    pcd8544_buffer[x+ (y/8)*LCDWIDTH] &= ~_BV(y%8); 

  updateBoundingBox(x,y,x,y);
}


// the most basic function, get a single pixel
uint8_t PCD8544::getPixel(uint8_t x, uint8_t y) {
  if ((x >= LCDWIDTH) || (y >= LCDHEIGHT))
    return 0;

  return (pcd8544_buffer[x+ (y/8)*LCDWIDTH] >> (7-(y%8))) & 0x1;  
}

void PCD8544::init(void) {
  init(50);
}

void PCD8544::init(uint8_t contrast) {
  // set pin directions
  pinMode(_din, OUTPUT);
  pinMode(_sclk, OUTPUT);
  pinMode(_dc, OUTPUT);
  pinMode(_rst, OUTPUT);
  pinMode(_cs, OUTPUT);

  // toggle RST low to reset; CS low so it'll listen to us
  if (_cs > 0)
    digitalWrite(_cs, LOW);

  digitalWrite(_rst, LOW);
  _delay_ms(500);
  digitalWrite(_rst, HIGH);


  // get into the EXTENDED mode!
  command(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION );

  // LCD bias select (4 is optimal?)
  command(PCD8544_SETBIAS | 0x4);

  // set VOP
  if (contrast > 0x7f)
    contrast = 0x7f;

  command( PCD8544_SETVOP | contrast); // Experimentally determined


  // normal mode
  command(PCD8544_FUNCTIONSET);

  // Set display to Normal
  command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);

  // initial display line
  // set page address
  // set column address
  // write display data

  // set up a bounding box for screen updates

  updateBoundingBox(0, 0, LCDWIDTH-1, LCDHEIGHT-1);
  // Push out pcd8544_buffer to the Display (will show the AFI logo)
  display();
  //_delay_ms(1000);
  // Clear the display
  //clear();
  //display();
}

inline void PCD8544::spiwrite(uint8_t c) {
  shiftOut(_din, _sclk, MSBFIRST, c);
}

void PCD8544::command(uint8_t c) {
  digitalWrite(_dc, LOW);
  spiwrite(c);
}

void PCD8544::data(uint8_t c) {
  digitalWrite(_dc, HIGH);
  spiwrite(c);
}

void PCD8544::setContrast(uint8_t val) {
  if (val > 0x7f) {
    val = 0x7f;
  }
  command(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION );
  command( PCD8544_SETVOP | val); 
  command(PCD8544_FUNCTIONSET);
  
 }



void PCD8544::display(void) {
  uint8_t col, maxcol, p;
  
  for(p = 0; p < 6; p++) {
#ifdef enablePartialUpdate
    // check if this page is part of update
    if ( yUpdateMin >= ((p+1)*8) ) {
      continue;   // nope, skip it!
    }
    if (yUpdateMax < p*8) {
      break;
    }
#endif

    command(PCD8544_SETYADDR | p);


#ifdef enablePartialUpdate
    col = xUpdateMin;
    maxcol = xUpdateMax;
#else
    // start at the beginning of the row
    col = 0;
    maxcol = LCDWIDTH-1;
#endif

    command(PCD8544_SETXADDR | col);

    for(; col <= maxcol; col++) {
      //uart_putw_dec(col);
      //uart_putchar(' ');
      data(pcd8544_buffer[(LCDWIDTH*p)+col]);
    }
  }

  command(PCD8544_SETYADDR );  // no idea why this is necessary but it is to finish the last byte?
#ifdef enablePartialUpdate
  xUpdateMin = LCDWIDTH - 1;
  xUpdateMax = 0;
  yUpdateMin = LCDHEIGHT-1;
  yUpdateMax = 0;
#endif

}

// clear everything
void PCD8544::clear(void) {
  memset(pcd8544_buffer, 0, LCDWIDTH*LCDHEIGHT/8);
  updateBoundingBox(0, 0, LCDWIDTH-1, LCDHEIGHT-1);
  cursor_y = cursor_x = 0;
}

/*
// this doesnt touch the buffer, just clears the display RAM - might be handy
void PCD8544::clearDisplay(void) {
  
  uint8_t p, c;
  
  for(p = 0; p < 8; p++) {

    st7565_command(CMD_SET_PAGE | p);
    for(c = 0; c < 129; c++) {
      //uart_putw_dec(c);
      //uart_putchar(' ');
      st7565_command(CMD_SET_COLUMN_LOWER | (c & 0xf));
      st7565_command(CMD_SET_COLUMN_UPPER | ((c >> 4) & 0xf));
      st7565_data(0x0);
    }     
    }

}

*/
