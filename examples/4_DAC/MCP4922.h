/*
 * MCP4922.h
 *
 * Library extension example: interface for dual 12-bit SPI DAC
 *      
 *           MCP4922            *
 *         _____ _____          *
 *        |     U     |         *
 *    VDD[| 1      14 |]VOUTA   *
 *     NC[| 2      13 |]VREFA   *
 *    CS'[| 3      12 |]AVSS    *
 *    SCK[| 4      11 |]VREFB   *
 *    SDI[| 5      10 |]VOUTB   *
 *     NC[| 6       9 |]SHDN'   *
 *     NC[| 7       8 |]LDAC'   *
 *        |___________|         *
 *                              * 
 * MCP4922    Arduino               
 * ---------------------------------
 * VDD        +5V
 * CS'        CS_PIN (defined below)
 * SCK        SCK
 * SDI        MOSI                   
 * VREFA      +5V
 * AVSS       GND
 * VREFB      +5V
 *
 * MCP4922 control word  
 * --------------------------------------------------------
 * A'/B BUF GA' SHDN' D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
 * --------------------------------------------------------
 *  A'/B: DACA or DACB Select bit 
 *    1 = Write to DACB
 *    0 = Write to DACA
 *  BUF: VREF Input Buffer Control bit
 *    1 = Buffered
 *    0 = Unbuffered
 *  GA':Output gain select bit
 *    1 = 1x (VOUT = VREF * D/4096)
 *    0 = 2x (VOUT = 2 * VREF * D/4096)
 *  SHDN': Output Power Down Control bit
 *    1 = Output Power Down Control bit
 *    0 = Output buffer disabled, Output is high impedance
 *  D11:D0: DAC Data bits
 *     12 bit number which sets the output value. Contains a value between 0 and 4095.
*/
#ifndef MCP4922_H
#define MCP4922_H

#include "SPIMaster.h"

// Use Port B pin 0 as chip select pin
#define CS_DDR (DDRB)
#define CS_PORT (PORTB)
#define CS_PIN (PB0)   

#define CHAN (15)
#define BUFF (14)
#define GAIN (13)
#define SHDN (12)

/*
 * MCP4922 Dual 12-bit SPI DAC
 */
struct MCP4922 : public SPIMaster {

    /*
     * Constructor
     */
    MCP4922() : SPIMaster() {
      CS_DDR |= (1 << CS_PIN);  // Configure CS pin as output
    };

    /* 
     * Output to channel A or B
     */
    void write_a(uint16_t sample) {
      CS_PORT &= ~(1 << CS_PIN);                // Assert CS (active low)
      write_u16(0b0011000000000000 | sample);   // Write control word
      CS_PORT |= (1 << CS_PIN);                 // Release CS
    }
    void write_b(uint16_t sample) {
      CS_PORT &= ~(1 << CS_PIN);
      write_u16(0b1011000000000000 | sample);
      CS_PORT |= (1 << CS_PIN);
    }
};

#endif  // MCP4922_H
