/*
 * Quad16.h
 * 
 * Library extension example: quadrature oscillator
 * 
 */

#ifndef QUAD16_H
#define QUAD16_H

#include "Oscillator.h"
#include "tables/sine_u16x1024.h"

/*
 * Quadrature oscillator
 * - Inherits from Wavetable16
 * - Uses a sine wave table to render sine and cosine
 */
struct Quad16 : public Wavetable16 {

  /*
   * Constructor
   */
  Quad16() : Wavetable16(sine_u16x1024, 6) {
    ; // Do nothing
  }

  /*
   * Render cosine and sine from table, return sine
   */
  uint16_t render() {
    /* Render a cosine from the sine table using the sine's phase, offset by (2^16)/4,
     * which corresponds to pi/2 or 90 degrees */
    cosine = (uint16_t)pgm_read_ptr(&sine_u16x1024[0] + ((phasor + (1 << 14)) >> shift));
    Wavetable16::render();    // Render a sine to the 'sample' variable
    return sample;
  }

  uint16_t cosine;  // Current cosine sample
};

#endif
