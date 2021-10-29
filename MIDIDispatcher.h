/*
  MIDIDispatcher.h

  Basic MIDI message handling and routing to user-supplied callbacks.
  
  Copyright (C) 2021 Jeff Gregorio
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Founddation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/  

/*
 * MIDIDispatcher:
 *  State machine for parsing incoming MIDI bytes and dispatching to 
 *  user-defined handlers for each message type.
 *  
 * Details at https://learn.sparkfun.com/tutorials/midi-tutorial/all
 */

struct MIDIDispatcher {

    /*
     * MIDI status bytes
     */
    enum {
      NOTE_OFF = 0x80,
      NOTE_ON = 0x90,
      POLY_PRESSURE = 0xA0,
      CONTROL_CHANGE = 0xB0,
      PROGRAM_CHANGE = 0xC0,
      CHANNEL_PRESSURE = 0xD0,
      PITCH_BEND = 0xE0,
      SYSEX = 0xF0
    };

    /*
     * Constructor
     */
    MIDIDispatcher() : msg_type(0), channel(0), d_bytes({0, 0}), d_idx(0) {}

    /* 
     *  State machine: read an incoming MIDI byte
     */
    void read(uint8_t val) {

      // Status byte (MSB is 1)
      if (val > 0x7F) {   
        msg_type = val & 0xF0;
        channel = val & 0x0F;
      }
      // System exclusive
      else if (msg_type == SYSEX) {
        ; // Eh, do we need this?
      }
      // Data byte, one expected
      else if (msg_type == PROGRAM_CHANGE || msg_type == CHANNEL_PRESSURE) {
        if (d_idx == 0) {           // First/only byte
          d_bytes[0] = val;
          dispatch();
        }
      }
      // Data byte, two expected
      else {
        if (d_idx == 0) {           // First byte
          d_bytes[d_idx++] = val;
        }
        else if (d_idx == 1) {      // Second byte
          d_bytes[d_idx--] = val;
          dispatch();
        }
      }
    }

    // User callbacks, directly settable
    void (*note_handler)(uint8_t ch, uint8_t note, uint8_t vel);
    void (*poly_pressure_handler)(uint8_t ch, uint8_t note, uint8_t val);
    void (*control_change_handler)(uint8_t ch, uint8_t num, uint8_t val);
    void (*program_change_handler)(uint8_t ch, uint8_t val);
    void (*channel_pressure_handler)(uint8_t ch, uint8_t val);
    void (*pitch_bend_handler)(uint8_t ch, uint8_t msbyte, uint8_t lsbyte);

protected:

    /* 
     *  Dispatcher: send completed messages to handlers
     */
    void dispatch() {
      switch (msg_type) {
        case NOTE_ON:
          if (note_handler) 
            note_handler(channel, d_bytes[0], d_bytes[1]);
            break;
        case NOTE_OFF:
          d_bytes[1] = 0;   // Force velocity zero and call a single note on/off handler
          if (note_handler) 
            note_handler(channel, d_bytes[0], d_bytes[1]);
          break;
        case POLY_PRESSURE:
          if (poly_pressure_handler) 
            poly_pressure_handler(channel, d_bytes[0], d_bytes[1]);
          break;
        case CONTROL_CHANGE:
          if (control_change_handler) 
            control_change_handler(channel, d_bytes[0], d_bytes[1]);
          break;
        case PROGRAM_CHANGE:
          if (program_change_handler) 
            program_change_handler(channel, d_bytes[0]);
          break;
        case CHANNEL_PRESSURE:
          if (channel_pressure_handler) 
            channel_pressure_handler(channel, d_bytes[0]); 
          break;
        case PITCH_BEND:
          if (pitch_bend_handler) 
            pitch_bend_handler(channel, d_bytes[0], d_bytes[1]);
          break;
        case SYSEX:
          break;
        default:
          break;
      }
    }

    uint8_t msg_type;   // Message type of most recent status byte
    uint8_t channel;    // Channel of most recent status byte
    uint8_t d_bytes[2]; // Data byte array
    uint8_t d_idx;      // Current index in data byte array
};
