// =========================================================================================================================================
//                                                 Rotating Display RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

// ******************************************************************************************************************************************
//
//          This class manages the multiplexing of the LEDs by means of the RP2040 PIO (programmable IO)
//
//          The file "LED_multiplexer.pio" is the assembler PIO program. It was compiled with the online
//          compiler https://wokwi.com/tools/pioasm. The result is LED_multiplexer.pio.h
// 
// ******************************************************************************************************************************************

#ifndef LED_control_H
#define LED_control_H

#include <Arduino.h>
#include "hardware/pwm.h"
#include "LED_multiplexer.pio.h"

#define pixelsPerTurn 256                         // pixelsPerTurn
#define linesPerTurn = 7 * pixelsPerTurn					// lines per turn
#define lineOffset 120                            // offset

#define Hall 11
#define PWM0_A 21       // only for test purposes
//#define PWM2_B 21

class LED_control {
  private:
    uint64_t static _timePerTurn;												  // time per turn
    uint32_t static _timePerLine; 							          // time per line

    void static _HallIRQ(uint gpio, uint32_t events);			// HallIRQ Handler
    void _initHallIRQ();                                  // initiate Hall Interrupt
    void static _PixelIRQ();	                        		// called for every line
    void _initPixelIRQ();

    uint32_t static _lineCounter;       								  // counts the displayed lines
    uint32_t static _lineOffset;                          // offset

    void _initMultiplexer();
    void static _pioIRQ();                                // requests data
    void _startPattern();                                 // generate simple start pattern
    void static _fillFifo();                              // push full 64 LED data set to pio FIFOs
    
  public:
    PIO static pio;
    uint64_t static offset;                              // memory offset for pio programs
    uint static sm0, sm1;

    LED_control();                                       // Constructor

    void begin();                                        // init PIO multiplexer
                                                         // setup interrupts for LED control

    uint8_t static lines[8][1792][3];									   // 
    uint32_t static brightness;
};

#endif
