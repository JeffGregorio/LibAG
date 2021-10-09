/*
 * Hive76 Synth Workshop
 * =====================
 * Example 4: MIDI DIN-5 to CV/gate interface
 */

#include "Timer.h"
#include "MIDIDispatcher.h"
#include <SoftwareSerial.h>

// Pin mappings
const int PIN_RX = 2;
const int PIN_GATE = 4;

// Lowest key and number of keys handled
const uint8_t LOW_KEY = 29;   // F2
const uint8_t NUM_KEYS = 49;  // F2-F#6

/* CV calibration:
 *  4096 / 5V = 819.2 steps/V
 *  819.2 / 12 = 68.26 steps/semitone
 *  scale by 68.26 * 128
 */
//const uint32_t CV_CAL = 8738;
const uint32_t CV_CAL = 9148;   // Fudged by ear-tuning octaves

// Object instances
Timer1 timer1;                          // Timer1 instance for hi-res PWM
MIDIDispatcher dispatcher;              // MIDI Dispatcher instance
SoftwareSerial midi_rx(PIN_RX, NULL);   // Software Serial with RX only

/* 
 *  Configure pins, serial, MIDI, and timer
 */
void setup() {

  pinMode(PIN_GATE, OUTPUT);
  
  // MIDI baud rate
  midi_rx.begin(31250);
  dispatcher.note_handler = &note_in;   // Set the MIDI note handler

  // 12-bit PWM @ 16e6/1/4096 = 3.9kHz
  timer1.set_prescaler(1);
  timer1.init_pwm(12);    
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
  note = note > LOW_KEY ? note - LOW_KEY : 0;
  note = note < NUM_KEYS ? note : NUM_KEYS;
  cv_out = (note * CV_CAL) >> 7;
  
  if (vel > 0) {
    timer1.pwm_write_a(cv_out);
    digitalWrite(PIN_GATE, HIGH);
  }
  else {
    digitalWrite(PIN_GATE, LOW);
  }
}
