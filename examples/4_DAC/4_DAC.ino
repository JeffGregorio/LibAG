/*
 * LibAG Example 4: MCP4922 Dual 12-bit DAC
 * ---------------------------------------- 
 * - Outputs a sine wave to DAC channel A, cosine to channel B 
 *    - Uses quadrature oscillator class defined in Quad16.h
 * - Frequeny controlled with CV [0-5]V at ADC ch 0 (Arduino pin A0)
 * - Amplitude controlled with CV [0-5]V at ADC ch 1 (Arduino pin A1)
 * - Sample timing monitored at PD3 and PD2 (Arduino pins 3, and 2)
 * 
 * Required SPI connections (see MCP4922.h for detail)
 * ------------------------------------------------------
 * Name: Atmega328 Pin, Uno/Nano Pin | Name: MCP4922 Pin
 * ======================================================
 *  PB0:      12             D8      |  CS':     3
 *  SCK:      17             D13     |  SCK:     4
 * MOSI:      15             D11     |  SDI:     5
 */

#include <Timer.h>
#include <ADCAuto.h>
#include <PgmTable.h>
#include <FixedPoint.h>

#include <tables/exp1000_u16x1024.h>

#include "MCP4922.h"  // Subclass of SPIMaster
#include "Quad16.h"   // Subclass of Wavetable16

/* 
 * Timer 0 determines sample rate (fs = 16e6/8/100 = 20kHz)
 * - Use fs less than ADC free running rate
 */
const uint8_t T0_PS = 8;     // Prescaler
const uint8_t T0_OC = 99;    // Output compare value
const float fs = 16e6 / (float)T0_PS / (float)(T0_OC + 1);

/*
 * ADC prescaler determines maximum conversion rate 16e6/32/13 = ~38.5kHz
 * - Note: prescaler < 128 trades quality for speed
 */
const uint8_t ADC_PS = 32;

/*
 * Peripheral drivers
 */
MCP4922 dac;      // External SPI DAC (12-bit)
Timer0 timer0;    // Timer 0 (CTC, sample rate)
ADCTimer0 adc(2); // ADC (prameter inputs)

/*
 * Quadrature oscillator instance (see Quad16.h)
 */
Quad16 lfo;

/* 
 *  Exponential frequency lookup table [0.2, 200] Hz
 *  - exp1000_u16x1024 --> Factor of 1000 sweep, 16-bit table, 10 bit length 
 *  - 200.0f/fs * 0xFFFF --> max freq 200Hz (normalized to 16-bit resolution)
 */
PgmTable16 freq_table(exp1000_u16x1024, 200.0f/fs * 0xFFFF);

/*
 * Setup
 */
void setup() {

  // Timing pins
  DDRD |= (1 << PD2) | (1 << PD3); 

  // CTC
  timer0.set_prescaler(T0_PS);
  timer0.init_ctc(T0_OC);

  // DAC
  dac.init();  

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

  uint16_t sine;
  uint16_t cosine;

  // Toggle/set timing pins
  PORTD ^= (1 << PD3);  
  PORTD |= (1 << PD2);

  // Update ADC conversions
  adc.update();

  // Set the LFO rate from the lookup table
  lfo.freq = freq_table.lookup_scale(adc.results[0]);
  
  // Render 
  sine = lfo.render();  
  cosine = lfo.cosine;     
  
  // Scale                 
  cosine = qmul16(cosine, adc.results[1] << 6);
  sine = qmul16(sine, adc.results[1] << 6);  

  // Write samples to DAC, right-shifted by 4 bits for 12-bit output
  dac.write_a(sine >> 4);
  dac.write_b(cosine >> 4);

  PORTD &= ~(1 << PD2);
}
