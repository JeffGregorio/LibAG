/*
 * LibAG Example 0: MIDI DIN-5 to CV/gate interface
 * ------------------------------------------------
 * - MIDI RX on pin 2 via Software Serial
 * - V/oct CV output via 12-bit PWM on OCR1A (Arduino pin 9)
 * - Gate output (digital) on PD4 (Arduino pin 4)
 * - Handles basic note on/off messages
 */

#include <Timer.h>
#include <MIDIDispatcher.h>
#include <SoftwareSerial.h>

/* 
 *  Pin mappings
 */
const int PIN_RX = 2;     // MIDI input
const int PIN_GATE = 4;   // Gate output

/*
 * Timer 1 determines PWM rate (16e6/1/4096 = 3.90625kHz) and resolution
 */
const uint8_t T1_PS = 1;      // Prescaler
const uint8_t T1_RES = 12;    // Bit resolution (ICR = (1 << T1_RES)-1)


/* 
 * Note range: lowest key and number of keys handled 
 */
const uint8_t LOW_KEY = 29;   // F2
const uint8_t NUM_KEYS = 49;  // F2-F#6

/* CV calibration:
 *  4096 / 5V = 819.2 steps/V
 *  819.2 / 12 = 68.26 steps/semitone
 *  scale by 68.26 * 128 = 8738
 *  - Note: actual scale determined by starting with 8738, then playing octaves
 *    and tuning the value until they sounded like octaves.
 */
const uint32_t CV_CAL = 9148;   

/*
 * Peripheral drivers
 */
Timer1 timer1;       

/*
 * MIDI message dispatcher and serial input controller
 */
MIDIDispatcher dispatcher;              
SoftwareSerial midi_rx(PIN_RX, NULL);   // Software Serial with RX only

/* 
 *  Setup
 */
void setup() {

  // Gate pin
  DDRD |= (1 << PD4);   // Output

  // PWM
  timer1.set_prescaler(T1_PS);
  timer1.init_pwm(T1_RES); 
  
  // MIDI baud rate
  midi_rx.begin(31250);

  // MIDI handler(s)
  dispatcher.note_handler = &note_in;   // MIDI notes
}

/*
 * Read from software serial and send bytes individually to the MIDI dispatcher
 */
void loop() {
  if (midi_rx.available() > 0) {
    dispatcher.read(midi_rx.read());
  }
}

/*
 * User handler (callback function) for incoming MIDI notes
 */
void note_in(uint8_t ch, uint8_t note, uint8_t vel) {

  uint16_t cv_out;

  // Constrain notes to specified range to restrict CV output
  note = note > LOW_KEY ? note - LOW_KEY : 0;
  note = note < NUM_KEYS ? note : NUM_KEYS;

  // Scale MIDI note number to CV
  cv_out = (note * CV_CAL) >> 7;

  // Note OFF handled as note ON with velocity = 0
  if (vel == 0) {
    PORTD &= ~(1 << PD4);       // Set gate LOW
  }
  else {
    timer1.pwm_write_a(cv_out); // Write CV
    PORTD |= (1 << PD4);        // Set gate HHIGH
  }
}
