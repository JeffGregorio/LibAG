/*
 * FixedPoint.h
 *
 * Fixed point math utility functions. Includes unsigned and signed
 * 8, 16, and 32-bit saturating addition, subtraction, and Q multiplicaiton.
 * 
 * Copyright (C) 2021 Jeff Gregorio
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

#define MAX_U32 0xFFFFFFFF
#define MAX_S32 0x7FFFFFFF
#define MIN_S32 0x80000000
#define MAX_U16 0xFFFF
#define MAX_S16 0x7FFF
#define MIN_S16 0x8000
#define MAX_U8 0xFF
#define MAX_S8 0x7F
#define MIN_S8 0x80

/*
 * Saturating addition (unsigned)
 */
// 0.8us
uint8_t addsat8(uint8_t a, uint8_t b) {
  uint8_t c = a + b;
  return c < a ? MAX_U8 : c;
}
// 1.3us
uint16_t addsat16(uint16_t a, uint16_t b) {
  uint16_t c = a + b;
  return c < a ? MAX_U16 : c;
}
// 2.4us
uint32_t addsat32(uint32_t a, uint32_t b) {
  uint32_t c = a + b;
  return c < a ? MAX_U32 : c;
}

/*
 * Saturating subtraction (unsigned)
 */
// 0.9us
uint8_t subsat8(uint8_t a, uint8_t b) {
  uint8_t c = a - b;
  return c > a ? 0 : c;
}
// 1.4us
uint16_t subsat16(uint16_t a, uint16_t b) {
  uint16_t c = a - b;
  return c > a ? 0 : c;
}
// 2.5us
uint32_t subsat32(uint32_t a, uint32_t b) {
  uint32_t c = a - b;
  return c > a ? 0 : c;
}

/*
 * Saturating addition (unsigned, signed)
 */
// 0.8us
int8_t addsat8(uint8_t a, int8_t b) {
  int8_t c = a + b;
  return c < b ? MAX_S8 : c;
}
// 1.3us
int16_t addsat16(uint16_t a, int16_t b) {
  int16_t c = a + b;
  return c < b ? MAX_S16 : c;
}
// 2.4us
int32_t addsat32(uint32_t a, int32_t b) {
  int32_t c = a + b;
  return c < b ? MAX_S32 : c;
}

/*
 * Saturating addition (signed)
 * - First conditional checks for potential overflow (operands of same sign)
 * - Second conditional checks for overflow (result different sign than operands)
 * - Ternary saturates with direction based on operand sign
 */
// 0.9 - 1.1us
int8_t addsat8(int8_t a, int8_t b) {
  int8_t c = a + b;
  if (((a ^ b) & MIN_S8) == 0) {    
    if ((c ^ a) & MIN_S8)            
      c = (a < 0) ? MIN_S8 : MAX_S8;  
  }
  return c;
}
// 1.4 - 2us
int16_t addsat16(int16_t a, int16_t b) {
  int16_t c = a + b;
  if (((a ^ b) & MIN_S16) == 0) {
    if ((c ^ a) & MIN_S16)
      c = (a < 0) ? MIN_S16 : MAX_S16;
  }
  return c;
}

// 2.4 - 3.5us
int32_t addsat32(int32_t a, int32_t b) {
  int32_t c = a + b;
  if (((a ^ b) & MIN_S32) == 0) {
    if ((c ^ a) & MIN_S32)
      c = (a < 0) ? MIN_S32 : MAX_S32;
  }
  return c;
}

/*
 * Q multiplication (unsigned)
 * - Note: all multiplications are rounded (i.e. round(x) = floor(x + 0.5))
 *  - by adding 1 << (K-1) before right-shifting by K 
 */
// 0.8us
uint8_t qmul8(uint8_t a, uint8_t b) {
  return (uint16_t)a * b + 0x80 >> 8;
}
// 2.9us
uint16_t qmul16(uint16_t a, uint16_t b) {
  return (uint32_t)a * b + 0x8000 >> 16;
}
// 18.1us
uint32_t qmul32(uint32_t a, uint32_t b) {
  return (uint64_t)a * b + 0x80000000 >> 32;
}

/*
 *  Q multiplication (unsigned, signed)
 */
// 0.9us
int8_t qmul8(uint8_t a, int8_t b) {
  return (int16_t)b * a + 0x80 >> 8;
}
// 3.8us
int16_t qmul16(uint16_t a, int16_t b) {
  return (int32_t)b * a + 0x8000 >> 16;
}
// 27us
int32_t qmul32(uint32_t a, int32_t b) {
  return (int64_t)b * a + 0x80000000 >> 32;
}

/*
 *  Q multiplication (signed)
 */
// 1.4uS
int8_t qmul8(int8_t a, int8_t b) {
  int16_t temp;
  temp = (int16_t)a * b + 0x40;
  temp = temp < 0x4000 ? temp : 0x3FFF;
  return temp >> 7;
}
// 11us
int16_t qmul16(int16_t a, int16_t b) {
  int32_t temp;
  temp = (int32_t)a * b + 0x4000;
  temp = temp < 0x40000000 ? temp : 0x3FFFFFFF;
  return temp >> 15;
}
// 26us
int32_t qmul32(int32_t a, int32_t b) {
  int64_t temp;
  temp = (int64_t)a * b + 0x40000000;
  temp = temp < 0x4000000000000000 ? temp : 0x3FFFFFFFFFFFFFFF;
  return temp >> 31;
}

#endif























