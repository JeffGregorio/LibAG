/*
 * Hive76 Synth Workshop
 * =====================
 * Example 1: Sinusoidal Low Frequency Oscillator
 */

#include "Timer.h"
#include "ADCAuto.h"
#include "Oscillator.h"
#include "ParamTable.h"
#include "FixedPoint.h"   

#include "tables/sine16x10.h"
#include "tables/exp16x10_1000.h"

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
 * Peripheral controllers
 */
Timer0 timer0;    // Timer 0 (CTC, sample rate)
Timer1 timer1;    // Timer 1 (PWM, output)
ADCAuto adc(2);   // ADC (prameter inputs)

/*
 * Sine wave table oscillator
 * - Wavetable16 --> 16-bit phase/frequency resolution
 * - sine16x10 --> 16-bit amplitude resolution, 10-bit length
 * - 6 --> shift phasor by 6 bits to read from table
 */
Wavetable16 lfo(sine16x10, 6);

/* 
 *  Exponential frequency lookup table [0.2, 200] Hz
 *  - exp16x10_1000 --> 16-bit table, 10 bit length, factor of 1000 sweep 
 *  - 200.0f/fs * 0xFFFF --> max freq 200Hz (normalized to 16-bit resolution)
 */
ParamTable16 freq_table(exp16x10_1000, 200.0f/fs * 0xFFFF);

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
  adc.init_timer0();
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

  uint16_t sample;

  // Toggle/set timing pins
  PORTD ^= (1 << PD3);  
  PORTD |= (1 << PD2);

  // Update ADC conversions
  adc.update();

  // Set the LFO rate from the lookup table
  lfo.freq = freq_table.lookup(adc.result[0]);

  // Render and scale the LFO
  sample = lfo.render();                        // 16-bit
  sample = qmul16(sample, adc.result[1] << 6);  // 16-bit x 16-bit = 16-bit

  // Right-shift by 6 bits for 10-bit output
  timer1.pwm_write_a(sample >> 6);   

  PORTD &= ~(1 << PD2);
}
