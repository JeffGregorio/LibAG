/*
 * LibAG Example 3: Filtered square wave (low-pass and high-pass)
 * --------------------------------------------------------
 * - ADC conversions and processing triggered by Timer 0 at 10kHz
 * - Waveform frequency controlled with V [0-5V] at ADC ch 0 (Arduino pin A0)
 * - Cutoff frequency controlled with CV [0-5]V at ADC ch 1 (Arduino pin A1)
 * - Outputs LP and HP outputs via 10-bit PWM to OCR1A and OCR1B (Arduino pins 9 and 10)
 * - Sample timing monitored at PD3 and PD2 (Arduino pins 3, and 2)
 * 
 * - Recommend a Sallen-Key low pass reconstruction filter on pin OCR1A
 *   with fc ~= 313Hz for approx -50db attenuation at fs/2. Use
 *   R1 = R2 = 4.7k and C1 = C2 = 0.1uF.
 */

#include <Timer.h>
#include <ADCAuto.h>
#include <Oscillator.h>
#include <IIR.h>
#include <PgmTable.h>
#include <FixedPoint.h>

#include <tables/exp1000_u16x1024.h>
#include <tables/exp10000_u16x1024.h>

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
 * Sawtooth wave oscillator
 * - Phasor16 --> 16-bit phase/frequency resolution
 */
Phasor16 lfo;

/* 
*  Exponential frequency lookup table [0.2, 200] Hz
*  - exp1000_u16x1024 --> Factor of 1000 sweep, 16-bit table, 10 bit length 
*  - 200.0f/fs * 0xFFFF --> max freq 200Hz (normalized to 16-bit resolution)
*/
PgmTable16 freq_table(exp1000_u16x1024, 200.0f / fs * 0xFFFF);

/*
 * One pole low pass filter
 */ 
OnePole16 lpf;

/*
 * Exponential frequency lookup table [0.2, 2000] Hz
 * - The UQ16 frequency is a decent approximation for the filter coefficient
 * - exp10000_u16x1024 --> Factor of 10000 sweep, unsigned 16-bit table, 10 bit length
 *  - 2000.0f/fs * 0xFFFF --> max freq 2000Hz (normalized to 16-bit resolution)
 */
PgmTable16 coeff_table(exp10000_u16x1024, 2000.0f/fs * 0xFFFF);

/*
 * Setup
 */
void setup() {

  // Timing pins
  DDRD |= (1 << PD2) | (1 << PD3); 

  // CTC
  timer0.set_prescaler(T0_PS);
  timer0.init_ctc(T0_OC-1);

  // PWM
  timer1.set_prescaler(T1_PS);
  timer1.init_pwm(T1_RES);     

  // ADC
  adc.set_prescaler(ADC_PS);  
  adc.init();
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

  uint16_t u;
  int16_t s, a, b;

  // Toggle/set timing pins
  PORTD ^= (1 << PD3);  
  PORTD |= (1 << PD2);

  // Update ADC conversions
  adc.update();

  // Set the LFO rate from the lookup table
  lfo.freq = freq_table.lookup_scale(adc.results[0]);
  lpf.coeff = coeff_table.lookup_scale(adc.results[1]);

  // Render LFO, convert to square wave
  u = lfo.render();
  s = u > 0x7FFF ? -0x8000 : 0x7FFF; 
  
  a = lpf.process(s);   // Low returned from processing method
  b = lpf.hp;           // High pass output

  // Right-shift by 6 bits for 10-bit output
  timer1.pwm_write_a(a + 0x8000 >> 6);
  timer1.pwm_write_b(b + 0x8000 >> 6);  

  PORTD &= ~(1 << PD2);
}
