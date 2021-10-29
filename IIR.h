/*
  IIR.h

  Infinite inpulse response filter, Q16 (signed) fixed point.
  
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

#include "FixedPoint.h"

/* 
 * Signed 16-bit one-pole filter (or exponential weighted moving average; see
 * https://en.wikipedia.org/wiki/Low-pass_filter#Discrete-time_realization).
 * - Multimode variant with high pass output
 * - Uses UQ16 normalized frequency
 */
struct OnePole16 {

  /*
   * Constructor
   */
  OnePole16() : hp(0), lp(0), coeff(0) {
    ; // Do nothing
  }

  /*
   * Process a sample [-0x8000, 0x7FFF]
   */
  int16_t process(int16_t sample) {
    hp = (sample >> 1) - (lp >> 1);   // Q1.15
    lp += (int32_t)coeff * hp >> 15;  // Q16 x Q1.15 = Q16
    return lp;
  }

  /*
   * Data
   */
  int16_t hp, lp;   // High pass and low pass outputs
  uint16_t coeff;   // Coefficient
};

/* 
 * Signed 16-bit multimode one-pole filter, low-frequency extension
 * - Use instead of OnePoleQ16 to correct steady state errors at low frequencies
 *    - Scales signals to Q32 internally so 16-bit Q multiplication with small 
 *      coefficients still produces a signal to update LP output.
 */
struct OnePole16_LF {

  /*
   * Constructor
   */
  OnePole16_LF() : hp(0), lp(0), coeff(0) {
    ; // Do nothing
  }

  /*
   * Process a sample [-0x8000, 0x7FFF]
   */
  int16_t process(int16_t sample) {
    hp = ((int32_t)sample << 16) - lp;
    lp += (int64_t)coeff * hp >> 16;
    hp >>= 16;        // Store hp as Q16 for easy retrieval
    return lp >> 16;  // Return lp as Q16, keep it stored as Q32
  }

  /*
   * Data
   */
  int32_t hp, lp;   // High pass and low pass outputs
  uint16_t coeff;   // Coefficient
};

/*
 * One Pole multimode filter based on trapezoidal integration
 * see https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf
 */
struct TPTOnePole16 {

  /*
   * Constructor
   */
  TPTOnePole16() : lp(0), hp(0), state(0), coeff(0) {
    ; // Do nothing
  }

  /*
   * Process a sample [-0x8000, 0x7FFF]
   */
  int16_t process(int16_t sample) {
    int16_t hp_scaled;
    hp = sample - state;
    hp_scaled = (int32_t)coeff * hp >> 16;
    lp = hp_scaled + state;
    state = lp + hp_scaled;
    return lp;
  }

  int16_t lp, hp;
  int16_t state;
  uint16_t coeff;
};

/*
 * Trapezoidal one pole multimode filter, low-frequency extension
 */
struct TPTOnePole16_LF {

  /*
   * Constructor
   */
  TPTOnePole16_LF() : lp(0), hp(0), state(0), coeff(0) {
    ; // Do nothing
  }

  /*
   * Process a sample [-0x8000, 0x7FFF]
   */
  int16_t process(int16_t sample) {
    int32_t hp_scaled;
    hp = ((int32_t)sample << 16) - state;
    hp_scaled = (int64_t)coeff * hp >> 16;
    lp = hp_scaled + state;
    state = lp + hp_scaled;
    hp >>= 16;
    return lp >> 16;
  }

  int32_t lp, hp;
  int32_t state;
  uint16_t coeff;
};

#endif
