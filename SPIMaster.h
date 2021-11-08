/*
  SPIMaster.h

  Configures SPI interface for generic SPI DAC or the MCP4921/2
  
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

#ifndef SPIMASTER_H
#define SPIMASTER_H

// Signal --- Port -- Nano pin (Atmega328 pin)
#define SS    PB2 //  D10 (14) -- also OC1B
#define MOSI  PB3 //  D11 (15)
#define MISO  PB4 //  D12 (16)
#define SCK   PB5 //  D13 (17)

/* SPI_DAC - Base class for SPI devices
 * 
 * SPCR - SPI control register, bits 7-0:
 * 
 *  SPIE - Interrupt enable
 *  SPE - SPI enable
 *  
 *  DORD - Data direction: (0) MSB first, (1) LSB first
 *  MSTR - Master (1) or slave (0)
 *  
 *  CPOL CPHA - SPI mode
 *  - Clock polarity, i.e. data sampled on: (0) rising edge or (1) falling
 *  - Clock phase, i.e. data shifted on: (0) falling edge or (1) rising
 *  - 0 0 (mode 0)
 *  - 0 1 (mode 1)
 *  - 1 1 ("mode 2")
 *  - 1 0 ("mode 3")
 *  
 *  SPR1 SPRO - Clock divider
 *    - 4, 16, 64, 128
 *    
 * SPSR - SPI Status Register, bits 7-6:
 * - SPIF WCOL
 */
struct SPIMaster {

    /*
     * Constructor
     */
    SPIMaster() {
      ; // Do nothing
    };

    /*
     * Initialize SPI peripheral. Use SS pin as output.
     */
    void init() {
      DDRB |= (1 << SS) | (1 << MOSI) | (1 << SCK); // Output pins
      DDRD |= (1 << PD0);
      DDRB &= ~(1 << MISO);                         // Input pin
      SPCR |= (1 << MSTR);    // Master mode
      SPCR &= ~(1 << DORD);   // MSB first
      // Leave SPR1 SPR0 low (Prescaler 4, spi clock 4MHz)
      SPCR |= (1 << SPE);     // Enable SPI
    }

    /*
     * 8-bit write
     */
    void write_u8(uint8_t sample) {
      SPDR = sample;                  // Load data
      while (!(SPSR & (1 << SPIF)));  // Wait for transmission complete
    }

    /*
     * 16-bit write
     */
    void write_u16(uint16_t sample) {
      SPDR = (sample & (0xFF00)) >> 8;  // Write MSB
      while (!(SPSR & (1 << SPIF)));    // Wait
      SPDR = sample & 0xFF;             // Write LSB
      while (!(SPSR & (1 << SPIF)));    // Wait
    }
};

#endif
