/*
  ADCAuto.h

  ADC configuration for automatic conversions triggered by 
    - Timer 0 OCR0A (ADCTimer0)
    - External interrupt pin INT0 (ADCInt0)
    - Free running mode (ADCFreeRunning)

  All classes expect user to call the ADC class's update() method in 
  ISR(ADC_vect), which is called when automatic conversions complete.
  Samples can be retrieved from the results array.

  Triggering with OCRA or INT0 require the user to declare the respective
  interrupt service routines ISR(TIMER_COMPA_vect) and ISR(INT0_vect) or
  the ADC will not be triggered.
  
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
 * ADC base class for auto-triggered modes. Non-functional on its own. Use 
 * ADCTimer0, ADCInt0, or ADCFreeRunning.
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
   * Set the prescaler. Note datasheet recommendations to keep ADC clock
   * 16MHz/prescaler < 1MHz overall, and <200kHz for full 10-bit resolution.
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
   * Initialize auto-trigger
   */
  void init() {
    ADCSRA = adps;            // Init; set prescaler
    ADCSRB = 0;               // Init
    ADMUX = 0;                // Init
    ADMUX |= (1 << REFS0);    // Use AVcc as the reference   
    ADCSRA |= (1 << ADATE);   // Enabble auto trigger
    ADCSRA |= (1 << ADIE);    // Enable interrupts when measurement complete
    ADCSRA |= (1 << ADEN);    // Enable ADC
  }

  /*
   * Retrieve ADC results and store them in the buffer
   */
  uint8_t update() {
    uint8_t ch_out = ch;
    ch = ADMUX & 0x0F;                  // Get current channel before reading result
    results[ch] = ADCL | (ADCH << 8);   // Get ADC result (reading ADCL first)
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
  uint16_t results[8] = {0, 0, 0, 0, 0, 0, 0, 0};
};

/*
 * ADC timer 0 trigger mode, sample rate 16e6/prescaler(timer0)/OCR0A.
 * - User must include ISR(TIMER0_COMPA_vect) or ISR(ADC_vect) won't be called.
 */
struct ADCTimer0 : public ADCAuto {

  /*
   * Constructor
   */
  ADCTimer0(uint8_t n_ch) : ADCAuto(n_ch) {
    ; // Do nothing
  }

  /*
   * Initializer
   */
  void init() {
    cli();                                  // Disable interrupts
    ADCAuto::init();                        // Base class init
    ADCSRB |= (1 << ADTS1) | (1 << ADTS0);  // Trigger source OCR0A
    sei();                                  // Enable interrupts
  }  
};

/*
 * ADC external interrupt 0 trigger mode (rising edge pin INT0).
 * - User must include ISR(INT0_vect) or ISR(ADC_vect) won't be called.
 */
struct ADCInt0 : public ADCAuto {

  /*
   * Constructor
   */
  ADCInt0(uint8_t n_ch) : ADCAuto(n_ch) {
    ; // Do nothing
  }

  /*
   * Initializer
   */
  void init() {
    EIMSK |= (1 << INT0);                 // Enable INT0 interrupt
    EICRA |= (1 << ISC00) |(1 << ISC01);  // INT0 interrupt = rising level interrupt
    cli();                                // Disable interrupts
    ADCAuto::init();                      // Base class init
    ADCSRB |= (1 << ADTS1);               // Trigger external interrupt 0
    sei();                                // Enable interrupts
  }
};

/*
 * ADC free running mode, sample rate 16e6/presclaer/13.
 */
struct ADCFreeRunning : public ADCAuto {

  /*
   * Constructor
   */
  ADCFreeRunning(uint8_t n_ch) : ADCAuto(n_ch) {
    ; // Do nothing
  }

  /*
   * Initializer
   */
  void init() {
    cli();                  // Disable interrupts
    ADCAuto::init();        // Base class init (free-running by default)
    ADCSRA |= (1 << ADSC);  // Start first ADC measurement
    sei();                  // Enable interrupts
  }

  /*
   * Overridden update method
   * - For whatever reason, conversion results are offset by 1 in the buffer in 
   *   free running mode unless the channel is read after retrieving the result.
   */
  uint8_t update() {
    uint8_t ch_out = ch;
    results[ch] = ADCL | (ADCH << 8);   // Get ADC result (reading ADCL first)
    ch = ADMUX & 0x0F;                  // Get current channel after reading result
    if (ch == ch_max)                   // Select next channel in sequence
      ADMUX &= 0xF0;      
    else
      ADMUX++;
    return ch_out;
  }
};

#endif
