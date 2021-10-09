/*
   ADCAuto.h
  
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

#ifndef ADCAUTO_H
#define ADCAUTO_H

/*
 * ADC
 */
struct ADCAuto {

  /*
   * Constructor
   */
  ADCAuto(uint8_t n_ch) : ch(0), ch_max(n_ch-1) {
    // Disable up to 6 digital input buffers on pins A0-A5 
    for (int i = 0; i <= min(ch_max, 5); i++) {
      DIDR0 |= (1 << i);    
    }    
  }

  /*
   * Set the prescaler (only useful ones for free running mode)
   */
  void set_prescaler(uint8_t prescaler) {
    switch (prescaler) {
      case 2:
        adps = 0b001;
        break;
      case 4:
        adps = 0b010;
        break;
      case 8:
        adps = 0b011;
        break;
      case 16:
        adps = 0b100;
        break;
      case 32:            
        adps = 0b101;
        break;
      case 64:
        adps = 0b110;
        break;
      case 128:
      default:
        adps = 0b111;
        break;
    }
  }

  /*
   * Initialize auto-trigger from Timer0 OCRA
   */
  void init_timer0() {
    ADCSRA = adps;            // Init; set prescaler
    ADCSRB = 0;               // Init
    ADMUX = 0;                // Init
    ADMUX |= (1 << REFS0);    // Use AVcc as the reference   
    cli(); 
    ADCSRB |= (1 << ADTS1) | (1 << ADTS0);
    ADCSRA |= (1 << ADATE);   // Enabble auto trigger
    ADCSRA |= (1 << ADIE);    // Enable interrupts when measurement complete
    ADCSRA |= (1 << ADEN);    // Enable ADC
    sei();                    // Enable interrupts
  }

  
  /*
   * Initialize free running mode
   */
  void init_free_running() {
    ADCSRA = adps;            // Init; set prescaler
    ADCSRB = 0;               // Init
    ADMUX = 0;                // Init
    ADMUX |= (1 << REFS0);    // Use AVcc as the reference 
    cli();   
    ADCSRB = 0;               // Free running mode
    ADCSRA |= (1 << ADATE);   // Enabble auto trigger
    ADCSRA |= (1 << ADIE);    // Enable interrupts when measurement complete
    ADCSRA |= (1 << ADEN);    // Enable ADC
    ADCSRA |= (1 << ADSC);    // Start ADC measurements
    sei();                    // Enable interrupts
  }

  /*
   * Retrieve ADC results and store them in the buffer
   */
  uint8_t update() {
    uint8_t ch_out = ch;
    result[ch] = ADCL | (ADCH << 8);    // Get ADC result (reading ADCL first)
    ch = ADMUX & 0x0F;                  // Get current channel after reading result
    if (ch == ch_max)                   // Select next channel in sequence
      ADMUX &= 0xF0;      
    else
      ADMUX++;
    return ch_out;
  }

  /*
   * Data
   */
  uint8_t adps;           // Prescaler bits
  uint8_t ch;             // Current channel
  uint8_t ch_max;         // Highest channel #
  uint16_t result[8] = {0, 0, 0, 0, 0, 0, 0, 0};
};

#endif
