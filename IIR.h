/*
   IIR.h
  
  Copyright (C) 2021 Jeff Gregorio
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/  

#ifndef IIR_H
#define IIR_H

/* 
 * Unsigned 16-bit Exponential Weighted Moving Average
 */
struct OnePole16 {

  /*
   * Constructor
   */
  OnePole16() : val(0), coeff(0) {
    ; // Do nothing
  }

  /*
   * Process a sample 
   */
  uint16_t process(uint16_t sample) {
    if (sample > val) 
      val += coeff * (sample - val) >> 16;
    else
      val -= coeff * (val - sample) >> 16;
    return val;
  }

  /*
   * Data
   */
  uint16_t val;     // Current value
  uint32_t coeff;   
};

#endif
