/*
 * Envelope.h
 *
 * 32-bit fixed point linear envelope generator.
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

#ifndef ENVELOPE_H
#define ENVELOPE_H

struct ASR16 {

  /*
   * Enumeration of envelope states
   */
  enum {
    EnvStateIdle = 0,
    EnvStateAttack,
    EnvStateSustain,
    EnvStateRelease
  };

  /*
   * Constructor
   */
  ASR16() : state(EnvStateIdle), value(0), atk_rate(0x7FFF), rel_rate(0x7FFF) {
    ;   // Do nothing
  }

  /* 
   * Begin (HIGH) or end (LOW) a note
   */
  void gate(bool on) {
    if (on) 
      state = EnvStateAttack;
    else 
      state = EnvStateRelease;
  }

  /*
   * Render a sample after handling end of state conditions
   */
  uint32_t render() {
    uint16_t new_val;
    switch (state) {
      // Idle
      case EnvStateIdle:
        break;
      // Attack
      case EnvStateAttack:
        new_val = value + atk_rate;
        if (new_val < value) {    // End attack after overflow
          value = 0xFFFF;
          state = EnvStateSustain;
        }
        else
          value = new_val;
      // Sustain
      case EnvStateSustain:
        break;
      // Release
      case EnvStateRelease:
        new_val = value - rel_rate;
        if (new_val > value) {    // End release after unverflow
          value = 0;
          state = EnvStateIdle;
        }
        else 
          value = new_val;
        break;
      // Unknown
      default:
        break;
    }
    return value;
  }

  /*
   * Data
   */
  uint8_t state;      // Current state
  uint16_t value;     // Current output value
      
  // Parameters (directly settable)
  uint16_t atk_rate;  // Normalized attack rate
  uint16_t rel_rate;  // Normalized release rate 
};

#endif
