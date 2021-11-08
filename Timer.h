/*
 	Timer.h

 	Configures AVR Timers 0, 1, and 2 for Fast PWM or CTC modes.
 	
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

/* Timer output compare registers: OCR[Timer][Channel]
 *  =================================================================
 *  Register    | Atmega328 Pin (Uno Pin) | Atmega2560 Pin (Mega Pin)    
 *  =================================================================
 *  OCR0A       | 12 	(6)               | 26  (13)            
 *  OCR0B       | 11 	(5)               | 1 	(4)
 *  OCR1A       | 15 	(9)               | 24  (11)
 *  OCR1B       | 16 	(10)              | 25  (12)
 *  OCR1C 		| n/a 	(n/a) 			  | 26  (13)
 *  OCR2A       | 17 	(11)              | 23  (10)
 *  OCR2B       | 5 	(3)               | 18  (9)
 */

#ifndef TIMER_h
#define TIMER_h

/*
 * Timer 0, 8-bit 
 */
struct Timer0 {

	/*
	 * Constructor
	 */
	Timer0() : csbits(0) {
    	; // Do nothing
	}

	/*
	 * Set prescaler to control PWM rate or CTC interrupt timing
	 */
	void set_prescaler(uint16_t prescaler) {
		switch (prescaler) {
			case 1:
			default:
				csbits = 0b001;
				break;
			case 8:
				csbits = 0b010;
				break;
			case 64:
				csbits = 0b011;
				break;
			case 256:
				csbits = 0b100;
				break;
			case 1024:
				csbits = 0b101;
				break;
		}
	}

	/*
	 * Initialize in Fast PWM mode
	 */
	void init_pwm() {
    	// Enable output in OCR pins
#ifdef __AVR_ATmega328P__
		DDRD |= (1 << PD6);	// (Ch A) Arduino Uno pin 6
		DDRD |= (1 << PD5);	// (Ch B) Arduino Uno pin 5
#elif __AVR_ATmega2560__
		DDRB |= (1 << PB7); // (Ch A) Arduino Mega pin 13
		DDRG |= (1 << PB5); // (Ch B) Arduino Mega pin 4
#endif
		TCCR0A = 0;					// Clear control register A
		TCCR0B = 0;					// Clear control register B
		TCCR0A |= (1 << WGM01) | (1 << WGM00);	// Fast PWM (mode 3)
		TCCR0A |= (1 << COM0A1);	// Non-inverting mode (channel A)
		TCCR0A |= (1 << COM0B1);	// Non-inverting mode (channel B)
		TCCR0B |= csbits;			// Set clock prescaler bits
	}

	/*
	 * Initialize in CTC mode. Use ISR(TIMER0_COMPA_vect) {}.
	 */
	void init_ctc(uint8_t ocr0a) {
		TCCR0A = 0;					// Clear control register A
		TCCR0B = 0;					// Clear control register B
		TCCR0A |= (1 << WGM01);		// CTC (mode 2)
		TIMSK0 |= (1 << OCIE0A);	// Interrupt on OCR0A
		TCCR0B |= csbits;			// Set clock prescaler bits
		cli();						// Disable interrupts
		TCNT0 = 0;					// Initialize counter
		OCR0A = ocr0a;				// Set counter match value
		sei();						// Enble interrupts
	}

	/*
	 * Write PWM signal to OCR pins
	 */
	void pwm_write_a(uint8_t val) {	OCR0A = val; }
	void pwm_write_b(uint8_t val) {	OCR0B = val; }

	/*
	 * Data
	 */
	uint8_t csbits;	// Prescaler bits
};

/*
 * Timer 1, 16-bit 
 */
struct Timer1 {

	/*
	 * Constructor
	 */
	Timer1() : csbits(0) {
    	; // Do nothing
	}

	/*
	 * Set prescaler to control PWM rate or CTC interrupt timing
	 */
	void set_prescaler(uint16_t prescaler) {
		switch (prescaler) {
			case 1:
			default:
				csbits = 0b001;
				break;
			case 8:
				csbits = 0b010;
				break;
			case 64:
				csbits = 0b011;
				break;
			case 256:
				csbits = 0b100;
				break;
			case 1024:
				csbits = 0b101;
				break;
		}
	}

	/*
	 * Initialize in Fast PWM mode
	 */
	void init_pwm(uint8_t bit_res = 8) {
    	uint16_t icr = (1 << bit_res) - 1;
    	// Enable output in OCR pins
#ifdef __AVR_ATmega328P__
		DDRB |= (1 << PB1);	// (Ch A) Arduino Uno pin 10
		DDRB |= (1 << PB2);	// (Ch B) Arduino Uno pin 9
#elif __AVR_ATmega2560__
		DDRB |= (1 << PB5);	// (Ch A) Arduino Mega pin 11
		DDRB |= (1 << PB6);	// (Ch B) Arduino Mega pin 12
		DDRB |= (1 << PB7); // (Ch C) Arduino Mega pin 13
#endif
    	ICR1H = (icr & 0xFF00) >> 8;
		ICR1L = icr & 0xFF;
		TCCR1A = 0;					// Clear control register A
		TCCR1B = 0;					// Clear control register B
		TCCR1A |= (1 << WGM11);		// Fast PWM (mode 14)
		TCCR1B |= (1 << WGM12) | (1 << WGM13);
		TCCR1A |= (1 << COM1A1);	// Non-inverting mode (channel A)
		TCCR1A |= (1 << COM1B1);	// Non-inverting mode (channel B)
		TCCR1B |= csbits;			// Set clock prescaler bits
	}

	/*
	 * Initialize in CTC mode. Use ISR(TIMER1_COMPA_vect) {}.
	 */
	void init_ctc(uint16_t ocr1a) {
		TCCR1A = 0;					// Clear control register A
		TCCR1B = 0;					// Clear control register B
		TCCR1B |= (1 << WGM12);		// CTC (mode 4)
		TIMSK1 |= (1 << OCIE1A);	// Interrupt on OCR1A
		TCCR1B |= csbits;			// Set clock prescaler bits
		cli();						// Disable interrupts
		TCNT1L = TCNT1H = 0;		// Initialize counter
		OCR1AH = (ocr1a & 0xFF00) >> 8;		// Set counter match value
	  	OCR1AL = ocr1a & 0xFF; 
		sei();						// Enble interrupts
	}

	/*
	 * Write PWM signal to OCR pins
	 */
	void pwm_write_a(uint16_t val) {	
    	OCR1AH = (val & 0xFF00) >> 8;
	  	OCR1AL = val & 0xFF; 
	}
	void pwm_write_b(uint16_t val) {	
    	OCR1BH = (val & 0xFF00) >> 8;
	 	OCR1BL = val & 0xFF; 
	}
#ifdef __AVR_ATmega2560__
	void pwm_write_c(uint16_t val) {
		OCR1CH = (val & 0xFF00) >> 8;
		OCR1CL = val & 0xFF;
	}
#endif

	/*
	 * Data
	 */
	uint8_t csbits;	// Prescaler bits
};

/*
 * Timer 2, 8-bit
 */
struct Timer2 {

	/*
	 * Constructor
	 */
	Timer2() : csbits(0) {
    	; // Do nothing
	}

	/*
	 * Set prescaler to control PWM rate or CTC interrupt timing
	 */
	void set_prescaler(uint16_t prescaler) {
		switch (prescaler) {
			case 1:
			default:
				csbits = 0b001;
				break;
			case 8:
				csbits = 0b010;
				break;
			case 32:
				csbits = 0b011;
				break;
			case 64:
				csbits = 0b100;
				break;
			case 128:
				csbits = 0b101;
				break;
			case 256:
				csbits = 0b110;
				break;
			case 1024:
				csbits = 0b111;
				break;
		}
	}

	/*
	 * Initialize in Fast PWM mode
	 */
	void init_pwm() {
// Enable output in OCR pins
#ifdef __AVR_ATmega328P__
		DDRD |= (1 << PD3);	// (Ch A) Arduino Uno pin 11
		DDRB |= (1 << PB3);	// (Ch B) Arduino Uno pin 3
#elif __AVR_ATmega2560__
		DDRB |= (1 << PB4);	// (Ch A) Arduino Mega pin 10
		DDRH |= (1 << PH6);	// (Ch B) Arduino Mega pin 9
#endif
		TCCR2A = 0;					// Clear control register A
		TCCR2B = 0;					// Clear control register B
		TCCR2A |= (1 << WGM21) | (1 << WGM20);	// Fast PWM (mode 3)
		TCCR2A |= (1 << COM2A1);	// Non-inverting mode (channel A)
		TCCR2A |= (1 << COM2B1);	// Non-inverting mode (channel B)
		TCCR2B |= csbits;			// Set clock prescaler bits
	}

	/*
	 * Initialize in CTC mode. Use ISR(TIMER2_COMPA_vect) {}.
	 */
	void init_ctc(uint8_t ocr2a) {
		TCCR2A = 0;					// Clear control register A
		TCCR2B = 0;					// Clear control register B
		TCCR2A |= (1 << WGM21);		// CTC (mode 2)
		TIMSK2 |= (1 << OCIE2A);	// Interrupt on OCR2A
		TCCR2B |= csbits;			// Set clock prescaler bits
		cli();						// Disable interrupts
		TCNT2 = 0;					// Initialize counter
		OCR2A = ocr2a;				// Set counter match value
		sei();						// Enble interrupts
	}

	/*
	 * Write PWM signal to OCR pins
	 */
	void pwm_write_a(uint8_t val) {	OCR2A = val; }
	void pwm_write_b(uint8_t val) {	OCR2B = val; }

	/*
	 * Data
	 */
	uint8_t csbits;	// Prescaler bits
};

#endif
