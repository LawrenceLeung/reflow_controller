/*
 Print.cpp - Base class that provides print() and println()
 Copyright (c) 2008 David A. Mellis.  All right reserved.
 
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
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 Modified 23 November 2006 by David A. Mellis
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <avr/pgmspace.h>
#include "wiring.h"

#include "Print.h"

// Public Methods //////////////////////////////////////////////////////////////

void Print::write(const char *str)
{
  while (*str)
    write(*str++);
}

void Print::write(const uint8_t *buffer, size_t size)
{
  while (size--)
    write(*buffer++);
}

void Print::print(const String &s)
{
  unsigned int len = s.length();
  for (unsigned int i=0; i < len; i++) {
    write(s[i]);
  }
}

void Print::print(uint8_t b)
{
  this->write(b);
}

void Print::print(char c)
{
  print((byte) c);
}

void Print::print(const char c[])
{
  write(c);
}

void Print::print(const __FlashStringHelper *ifsh)
{
	const prog_char *p = (const prog_char *)ifsh;
	while (1) {
		unsigned char c = pgm_read_byte(p++);
		if (c == 0) return;
		write(c);
	}
}

void Print::print(int n)
{
  print((long) n);
}

void Print::print(unsigned int n)
{
  print((unsigned long) n);
}

void Print::print(long n)
{
	uint8_t sign=0;

	if (n < 0) {
		sign = '-';
		n = -n;
	}
	printNumber(n, 10, sign);
}

void Print::print(unsigned long n)
{
	printNumber(n, 10, 0);
}

void Print::print(long n, int base)
{
  if (base == 0)
    print((char) n);
  else if (base == 10)
    print(n);
  else
    printNumber(n, base, 0);
}

void Print::print(double n)
{
  printFloat(n, 2);
}

void Print::println(void)
{
	uint8_t buf[2]={'\r', '\n'};
	write(buf, 2);
}

void Print::println(const String &s)
{
  print(s);
  println();
}

void Print::println(char c)
{
  print(c);
  println();  
}

void Print::println(const char c[])
{
  print(c);
  println();
}

void Print::println(const __FlashStringHelper *ifsh)
{
  print(ifsh);
  println();
}

void Print::println(uint8_t b)
{
  print(b);
  println();
}

void Print::println(int n)
{
  print(n);
  println();
}

void Print::println(unsigned int n)
{
  print(n);
  println();
}

void Print::println(long n)
{
  print(n);
  println();  
}

void Print::println(unsigned long n)
{
  print(n);
  println();  
}

void Print::println(long n, int base)
{
  print(n, base);
  println();
}

void Print::println(double n)
{
  print(n);
  println();
}

// Private Methods /////////////////////////////////////////////////////////////

void Print::printNumber(unsigned long n, uint8_t base, uint8_t sign)
{
	uint8_t buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars.
	uint8_t digit, i;

	if (n == 0) {
		buf[sizeof(buf) - 1] = '0';
		i = sizeof(buf) - 1;
	} else {
		i = sizeof(buf) - 1;
		while (1) {
			digit = n % base;
			buf[i] = ((digit < 10) ? '0' + digit : 'A' + digit - 10);
			n /= base;
			if (n == 0) break;
			i--;
		}
	}
	if (sign) {
		i--;
		buf[i] = sign;
	}
	write(buf + i, sizeof(buf) - i);
}

void Print::printFloat(double number, uint8_t digits) 
{
  uint8_t sign=0;

  // Handle negative numbers
  if (number < 0.0) {
     sign = '-';
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i)
    rounding /= 10.0;
  
  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  printNumber(int_part, 10, sign);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0) {
	uint8_t n, buf[5], count=1;
	buf[0] = '.';

	//write('.'); 
	// Extract digits from the remainder one at a time
	if (digits > sizeof(buf) - 1) digits = sizeof(buf) - 1;

	while (digits-- > 0) {
		remainder *= 10.0;
		n = (uint8_t)(remainder);
		buf[count++] = '0' + n;
		remainder -= n; 
	}
	write(buf, count);
  }
}





