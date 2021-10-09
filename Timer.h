/*
 	Timer.h
 	
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
 *  ==================================================
 *  Register    | Atmega328 Pin     | Arduino Pin 
 *  ==================================================
 *  OCR0A       | 12                | 6             
 *  OCR0B       | 11                | 5
 *  OCR1A       | 15                | 9
 *  OCR1B       | 16                | 10
 *  OCR2A       | 17                | 11
 *  OCR2B       | 5                 | 3
 */

#ifndef TIMER_h
#define TIMER_h

// =============================
// ---------- Timer 0 ----------
// =============================
struct Timer0 {

	Timer0() : csbits(0) {
    ; // Do nothing
	}

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

	void init_pwm() {
		DDRD |= (1 << PD6);	// Enable output on channel A (Port D, bit 6, or pin 6)
		DDRD |= (1 << PD5);	// Enable output on channel B (Port D, bit 5, or pin 5)
		TCCR0A = 0;					// Clear control register A
		TCCR0B = 0;					// Clear control register B
		TCCR0A |= (1 << WGM01) | (1 << WGM00);	// Fast PWM (mode 3)
		TCCR0A |= (1 << COM0A1);	// Non-inverting mode (channel A)
		TCCR0A |= (1 << COM0B1);	// Non-inverting mode (channel B)
		TCCR0B |= csbits;			// Set clock prescaler bits
	}

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

	void pwm_write_a(uint8_t val) {	OCR0A = val; }
	void pwm_write_b(uint8_t val) {	OCR0B = val; }

	uint8_t csbits;
};

// =============================
// ---------- Timer 1 ----------
// =============================
struct Timer1 {

	Timer1() : csbits(0) {
    ; // Do nothing
	}

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

	void init_pwm(uint8_t bit_res = 8) {
    	uint16_t icr = (1 << bit_res) - 1;
		DDRB |= (1 << PB1);	// Enable output on channel A (Port B, bit 1, or pin 9)
		DDRB |= (1 << PB2);	// Enable output on channel B (Port B, bit 2, or pin 10)
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

	void pwm_write_a(uint16_t val) {	
    	OCR1AH = (val & 0xFF00) >> 8;
	  	OCR1AL = val & 0xFF; 
	}
	void pwm_write_b(uint16_t val) {	
    	OCR1BH = (val & 0xFF00) >> 8;
	  OCR1BL = val & 0xFF; 
	}

	uint8_t csbits;
};

// =============================
// ---------- Timer 2 ----------
// =============================
struct Timer2 {

	Timer2() : csbits(0) {
    ; // Do nothing
	}

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

	void init_pwm() {
		DDRD |= (1 << PD3);	// Enable output on channel A (Port D, bit 3, or pin 11)
		DDRB |= (1 << PB3);	// Enable output on channel B (Port B, bit 3, or pin 3)
		TCCR2A = 0;					// Clear control register A
		TCCR2B = 0;					// Clear control register B
		TCCR2A |= (1 << WGM21) | (1 << WGM20);	// Fast PWM (mode 3)
		TCCR2A |= (1 << COM2A1);	// Non-inverting mode (channel A)
		TCCR2A |= (1 << COM2B1);	// Non-inverting mode (channel B)
		TCCR2B |= csbits;			// Set clock prescaler bits
	}

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

	void pwm_write_a(uint8_t val) {	OCR2A = val; }
	void pwm_write_b(uint8_t val) {	OCR2B = val; }

	uint8_t csbits;
};

#endif
