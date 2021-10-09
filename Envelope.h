/*
 * Envelope.h
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

struct ASR32 {

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
  ASR32() : state(EnvStateIdle), phase(0), len(1), value(0), slope(0), 
            atk_len(2), sus_lev(0xFFFFFFFF), rel_len(2) {
    ;   // Do nothing
  }

  /* 
   * Begin (HIGH) or end (LOW) a note
   */
  void gate(bool on) {
    if (on) 
      begin_atk();
    else    
      begin_rel();
  }

  /*
   * Render a sample after handling end of state conditions
   */
  uint32_t render() {
    if (phase++ == len-1) {
      switch (state) {
        case EnvStateIdle:
        case EnvStateRelease:
          begin_idle();
          break;
        case EnvStateAttack:
        case EnvStateSustain:
          begin_sus();
          break;
        default:
          break;
      }
    }
    value += slope;
    return value;
  }

  /*
   * Internal state handlers
   */
  void begin_idle() {
    state = EnvStateIdle;
    len = 0xFFFFFFFF;
    phase = 0;
    slope = 0;
  }

  void begin_atk() {
    state = EnvStateAttack;
    len = atk_len;
    phase = 0;
    slope = (sus_lev - value) / atk_len;
  }
  
  void begin_sus() {
    state = EnvStateSustain;
    len = 0xFFFFFFFF;
    phase = 0;
    slope = 0;
  }

  void begin_rel() {
    state = EnvStateRelease;
    len = rel_len;
    phase = 0;
    slope = -(value / rel_len);
  }

  /*
   * Data
   */
  uint8_t state;      // Current state
  uint32_t phase;     // Phase in current state
  uint32_t len;       // Length of current state

  int32_t value;      // Current output value
  int32_t slope;      // Current slope
      
  // Parameters (directly settable)
  uint32_t atk_len;   // Attack state length in samples
  uint32_t sus_lev;   // Sustain level
  uint32_t rel_len;   // Release state length in samples  
};

#endif
