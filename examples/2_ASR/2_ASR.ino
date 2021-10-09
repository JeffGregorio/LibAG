/*
 * Hive76 Synth Workshop
 * =====================
 * Example 2: ASR Envelope Generator
 */

#include "Timer.h"
#include "ADCAuto.h"
#include "Envelope.h"
#include "ParamTable.h"
#include "FixedPoint.h"

#include "tables/exp16x10_100.h"

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
 * Envelope generator (32-bit amplitude/length resolution)
 */
ASR32 asr;

/* 
 *  Exponential envelope time lookup table [0.04, 4] sec 
 *  - exp16x10_100 --> 16-bit table, 10 bit length, factor of 100 sweep 
 *  - 4.0f*fs --> max sample length (corresponding to 4 sec)
 */
ParamTable16 len_table(exp16x10_100, 4.0f*fs);

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
  adc.init_timer0();

  // Gate pin
  DDRD &= ~(1 << PD4);    // Set D4 as input
  PORTD &= ~(1 << PD4);   // No pullup
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
  uint32_t sample;

  // Toggle/set timing pins
  PORTD ^= (1 << PD3);
  PORTD |= (1 << PD2);

  // Update ADC conversions
  adc.update();

  // Set envelope times from the lookup table
  asr.atk_len = len_table.lookup(adc.result[1]);
  asr.rel_len = len_table.lookup(adc.result[0]);

  // Gate on rising edge on pin D4
  gate_in = PIND & (1 << PD4);  
  if (state_gate != gate_in) {
    asr.gate(gate_in);
    state_gate = gate_in;
  }

  // Render the envelope
  sample = asr.render();

  // Right-shift by 22 bits to get 10-bit value 
  timer1.pwm_write_a(sample >> 22);  

  PORTD &= ~(1 << PD2);
}
