/*
 * Oscillator.h
 * 
 * UQ16 Fixed point sawtooth and wavetable oscillators.
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
 
#ifndef OSCILLATOR_H
#define OSCILLATOR_H

/* 16-bit Fixed Point phasor
/*  - Periodic ramp (phase accumulator) in range [0, 2*pi] = [0, 2^16-1]
 *  - Normalized frequency range [-pi, pi] = [-2^15, 2^15-1]
 *    - Freq > 0 --> ascending ramps
 *    - Freq < 0 --> descending ramps
 */
struct Phasor16 {

    // Create with specified output bit resolution
    Phasor16() : phase(0), freq(0), phasor(0) {
        ; // Do nothing
    }

    /*
     * Render a naive sawtooth sample
     */
    uint16_t render() {
        phasor = phase;
        phase += freq;
        return phasor;
    }

    /*
     * Data
     */
    uint16_t phase;         // Phase accumulator in [0, 2^16]
    int16_t freq;           // Phase increment in [-2^15, 2^15-1]
    uint16_t phasor;        // Naive sawtooth 
    
};

/* Wavetable oscillator
 *  Wave table lookup using a Phasor16's output scaled to the table length via
 *  right shift of specified length
 */
struct Wavetable16 : public Phasor16 {

    /*
     * Constructor for user-provided table and right shift length
     */
    Wavetable16(uint16_t *table, uint8_t shift) : Phasor16(), table(table), shift(shift) {
        ; // Do nothing
    }

    /*
     * Render a sample from the wavetable
     */
    uint16_t render() {
        sample = (uint16_t)pgm_read_ptr(&table[0] + (phasor >> shift));
        Phasor16::render();
        return sample;
    }

    /*
     * Data
     */
    uint16_t *table;
    uint8_t shift;
    uint16_t sample;
};

#endif
