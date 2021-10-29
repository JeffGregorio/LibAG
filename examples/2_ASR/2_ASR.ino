/*
 * LibAG Example 2: ASR Envelope Generator
 * ---------------------------------------
 * - ADC conversions and processing triggered by Timer 0 at 10kHz
 * - Outputs an envelope via 10-bit PWM to OCR1A (Arduino pin 9)
 * - Gate input at PD4 (Arduino pin 4)
 * - Attack time controlled with CV [0-5]V at ADC ch 0 (Arduino pin A0)
 * - Release time controlled with CV [0-5]V at ADC ch 1 (Arduino pin A1)
 * - Sample timing monitored at PD3 and PD2 (Arduino pins 3, and 2)
 * 
 * - Recommend a Sallen-Key low pass reconstruction filter on pin OCR1A
 *   with fc ~= 313Hz for approximately -50db attenuation at fs/2. Use
 *   R1 = R2 = 4.7k and C1 = C2 = 0.1uF.
 */

#include "Timer.h"
#include "ADCAuto.h"
#include "Envelope.h"
#include "PgmTable.h"
#include "FixedPoint.h"

#include "tables/exp100_u16x1024.h"

/* 
 * Timer 0 determines sample rate (fs = 16e6/8/200 = 10kHz)
 * - Use fs less than ADC free running rate
 * - Use fs less than PWM rate
 */
const uint8_t T0_PS = 8;      // Prescaler
const uint8_t T0_OC = 200;    // Output compare value
const float fs = 16e6 / (float)T0_PS / (float)T0_OC;

/*
 * Timer 1 determines PWM rate (16e6/1/1024 = 15.625kHz) and resolution
 */
const uint8_t T1_PS = 1;      // Prescaler
const uint8_t T1_RES = 10;    // Bit resolution (ICR = (1 << T1_RES)-1)

/*
 * ADC prescaler determines maximum conversion rate 16e6/64/13 = ~19.2kHz
 * - Note: prescaler < 128 trades quality for speed
 */
const uint8_t ADC_PS = 64;

/*
 * Peripheral drivers
 */
Timer0 timer0;    // Timer 0 (CTC, sample rate)
Timer1 timer1;    // Timer 1 (PWM, output)
ADCTimer0 adc(2); // ADC (prameter inputs)

/*
 * Envelope generator (16-bit amplitude/rate resolution)
 */
ASR16 asr;

/* 
 *  Exponential envelope time lookup table [0.04, 4] sec 
 *  - Corresponds to frequency range [25, 0.25] Hz
 *    - We can use [0.25, 25] and invert the control on lookup
 *  - exp100_u16x1024 --> Factor of 100 sweep, unsigned 16-bit table, 10 bit length 
 *  - 200.0f/fs * 0xFFFF --> max freq 25Hz (normalized to 16-bit resolution)
 */
PgmTable16 rate_table(exp100_u16x1024, 25.0f / fs * 0xFFFF);

// Hold the state of the envelope gate pin to detect changes
bool state_gate = 0;

/*
 * Setup
 */
void setup() {
  
  // Timing pins
  DDRD |= (1 << PD2) | (1 << PD3); 

  // CTC
  timer0.set_prescaler(T0_PS);
  timer0.init_ctc(T0_OC);

  // PWM
  timer1.set_prescaler(T1_PS);
  timer1.init_pwm(T1_RES);     

  // ADC
  adc.set_prescaler(ADC_PS);  
  adc.init();

  // Gate pin
  DDRD &= ~(1 << PD4);    // Set D4 as input
}

/*
 * Loop
 */
void loop() {
  ; // Do nothing
}

/* 
 * Note: this ISR must be included for timer 0 to trigger the ADC 
 */
ISR(TIMER0_COMPA_vect) {
  ; // Do nothing
}

/*
 * Convert, process, render, and output samples at sample rate
 */
ISR(ADC_vect) {

  bool gate_in;
  uint16_t sample;

  // Toggle/set timing pins
  PORTD ^= (1 << PD3);
  PORTD |= (1 << PD2);

  // Update ADC conversions
  adc.update();

  // Set envelope times from the lookup table
  asr.atk_rate = rate_table.lookup_scale(1023 - adc.results[0]);
  asr.rel_rate = rate_table.lookup_scale(1023 - adc.results[1]);
  
  // Gate on falling edge on pin D4
  gate_in = PIND & (1 << PD4);  
  if (state_gate != gate_in) {
    asr.gate(gate_in);
    state_gate = gate_in;
  }

  // Render the envelope
  sample = asr.render();

  // Right-shift by 22 bits to get 10-bit value 
  timer1.pwm_write_a(sample >> 6); 

  PORTD &= ~(1 << PD2);
}
