// =========================================================================================================================================
//                                                 Rotating Display RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include "LED_control.h"
#include "globals.h"
#include <Arduino.h>

// Definition of static variables
uint64_t LED_control::_timePerTurn = 0;
uint32_t LED_control::_timePerLine= 30000;
uint32_t LED_control::_lineCounter = 0;              // 0....1791
uint32_t LED_control::_lineOffset = lineOffset;
uint8_t LED_control::lines[8][1792][3] = {0};	     // 8 bytes with 8 LEDs each, 1792 subpixel lines, 3 colors
uint32_t LED_control::brightness = 0x100;
PIO LED_control::pio = pio1;                         // SdFat uses pio0 
uint LED_control::sm0 = 0;
uint LED_control::sm1 = 1;


// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

LED_control::LED_control() {
}

// =========================================================================================================================================
//                                                      begin Method
// =========================================================================================================================================

void LED_control::begin() {
    _initMultiplexer();
    _initHallIRQ();
    _initPixelIRQ();
}

void LED_control::_initMultiplexer(){

    // Load LED_multiplexer_sm0 and LED_multiplexer_sm1 into this PIO's instruction memory.
    uint offset0 = pio_add_program(pio, &LED_multiplexer_sm0_program);
    uint offset1 = pio_add_program(pio, &LED_multiplexer_sm1_program);

    // initiate confic structures c0 and c1
    pio_sm_config c0 = LED_multiplexer_sm0_program_get_default_config(offset0);
    pio_sm_config c1 = LED_multiplexer_sm1_program_get_default_config(offset1);

    // Map the sm0 OUT pin group to pins 4, 5, 6, 7 (row selection lines)
    sm_config_set_out_pins(&c0, 4, 4);
    // map the sm0 SET pins to pin 2, 3 (LE and OE)
    sm_config_set_set_pins(&c0, 2, 2);
    // Map the sm1 OUT pin group to pins 0 (data)
    sm_config_set_out_pins(&c1, 0, 1); 
    // map the sm1 SET pins to pin 1 (clock)
    sm_config_set_set_pins(&c1, 1, 1);


    // define status
    sm_config_set_mov_status (&c1, STATUS_TX_LESSTHAN , 1 );   

    // Join TX FIFO sm0 (2x4 32 bit words = 8 32 bit words)
    sm_config_set_fifo_join	(&c0, PIO_FIFO_JOIN_TX);	
    // Join TX FIFO sm1
    sm_config_set_fifo_join	(&c1, PIO_FIFO_JOIN_TX);	
   
    // Set pindir for all pins
    pio_sm_set_consecutive_pindirs(pio, sm0, 2, 6, true);
    // Set pindir for all pins
    pio_sm_set_consecutive_pindirs(pio, sm1, 0, 2, true);
   
    // set clk frequency to 130/5 MHz = 26 MHz. 
    sm_config_set_clkdiv_int_frac(&c0, 1, 0);  // 
    sm_config_set_clkdiv_int_frac(&c1, 1, 0);  // 

    // enable IRQ by setting IRQ0_INTE - interrupt enable register    pis_interrupt0 = PIO_INTR_SM0_LSB
    // this routes the PIO SM interrupts (IRQ0...IRQ7) to the system interrupts 7 and 8.
    pio_set_irq0_source_enabled(pio, pis_interrupt0 , true);
    //Set the handler in the NVIC. IRQ7 = PIO0_IRQ_0
    irq_set_exclusive_handler(7, _pioIRQ);  
    //enabling the PIO0_IRQ_0
    irq_set_enabled(7, true);   

    // Set this pin's GPIO function "pio"
    for (uint i=0; i<8; i++) pio_gpio_init(pio, i);

    // Load configuration to pio, and jump to the start of the program
    pio_sm_init(pio, sm0, offset0, &c0);
    pio_sm_init(pio, sm1, offset1, &c1);

    // Set the state machine running
    pio_sm_set_enabled(pio, sm0, true);
    pio_sm_set_enabled(pio, sm1, true);
}

void LED_control::_pioIRQ() {               // not needed in RS64, as 64 LEDs data can be stored at once in the FIFO
    pio_interrupt_clear(pio, 0);                               // clear interrupt
}

void LED_control::_initHallIRQ() {
    gpio_init(Hall);
    gpio_set_dir(Hall, GPIO_IN);
    gpio_set_function(Hall, GPIO_FUNC_SIO);								// software IO control (SIO)

	gpio_set_irq_enabled_with_callback(Hall, GPIO_IRQ_EDGE_FALL, true, &_HallIRQ);    // attach HallRIQ

    pwm_clear_irq(0);										// clear pwm interrupt
    pwm_set_irq_enabled(0, true);							// enable pwm interrupt

} 

void LED_control::_HallIRQ(uint gpio, uint32_t events) {
    _timePerTurn = _timePerTurn + pwm_get_counter(0);

	uint64_t newTimePerLine = _timePerTurn/1792;

	if (newTimePerLine>65500) newTimePerLine = 65500;

    uint64_t delta = labs(newTimePerLine - _timePerLine);
    if(delta<10) _timePerLine = (63*_timePerLine+newTimePerLine)/64;      // Otherwise make small corrections, only.
  	if((delta>=10)&&(delta<100)) _timePerLine = (15*_timePerLine+newTimePerLine)/16;
    if(delta>=100) _timePerLine = newTimePerLine;
    
    pwm_set_wrap (0, _timePerLine);							// upper threshold of counter 2
    pwm_clear_irq(0);										// clear pwm interrupt

 	_timePerTurn = 0;                             
  	_lineCounter = _lineOffset;  							// restart line_counter
    _lineCounter = _lineCounter%1792;
    _fillFifo();
}

void LED_control::_initPixelIRQ() {
    gpio_init(PWM0_A);										// set gpio as output
    gpio_set_dir(PWM0_A, true);								//

    gpio_set_function(PWM0_A, GPIO_FUNC_PWM);				// enable PWM pin

    pwm_set_wrap (2, _timePerLine);							// upper threshold of counter 0
    pwm_set_phase_correct (0, false);						// counter 0 will reset to 0, once upper threshold is reached

    pwm_set_clkdiv_int_frac(0, 1, 0);					    // set divisor for clock divider (system clock = 125 MHz)

    pwm_set_chan_level(0, 0, _timePerLine-1);			    // level is the mid threshold, when the output goes to zero

    pwm_set_enabled (0, true);								// enable PWM slice 0 (output pins 16 and 17)

    irq_set_exclusive_handler(PWM_IRQ_WRAP, _PixelIRQ);		// attach PixelIRQ() to the PWM interrupt
    irq_set_enabled(PWM_IRQ_WRAP, true);					// enanble irq

	_startPattern();
}

void LED_control::_PixelIRQ() {    
    _timePerTurn+=_timePerLine;

    _lineCounter++;
    _lineCounter = _lineCounter%1792;


    _fillFifo();
	pwm_clear_irq(0);										// clear interrupt
}

void LED_control::_fillFifo() {

    // check whether fifos are empty. Otherwise: do nothing.
    uint fillLevelSm0 = pio_sm_get_tx_fifo_level (pio, sm0);
    uint fillLevelSm1 = pio_sm_get_tx_fifo_level (pio, sm1);

    if ((fillLevelSm0==0)&&(fillLevelSm1==0)) {
        // fill FIFO with full LED data set (64 RGB LEDs, 8 sets of 24 bits)
        for (int i = 0; i<8; i++) {
            uint32_t control = (brightness<<4)|i;
            uint32_t byte0, byte1, byte2;
            if (BMP.clockOn) {
                byte0 = (lines[i][_lineCounter][0] & bmpTemplateLines[i][_lineCounter]) | bmpLines[i][_lineCounter][0];
                byte1 = (lines[i][_lineCounter][1] & bmpTemplateLines[i][_lineCounter]) | bmpLines[i][_lineCounter][1];
                byte2 = (lines[i][_lineCounter][2] & bmpTemplateLines[i][_lineCounter]) | bmpLines[i][_lineCounter][2];
            } else {
                byte0 = lines[i][_lineCounter][0];
                byte1 = lines[i][_lineCounter][1];
                byte2 = lines[i][_lineCounter][2];
            }
            uint32_t data = byte0 | (byte1 << 16) | (byte2 << 8); //00GGBBRR
            pio_sm_put(pio, sm0, control);          // push delay and row number into FIFO0
            pio_sm_put(pio, sm1, data);             // push LED data into FIFO1
        }
    }
}

void LED_control::_startPattern(){
 
    for (int x=0; x<1792; x++) {
        uint8_t data[3] = {0};
        if (x<598) data[0] = 0xFF;
        if ((x>=598)&&(x<1196)) data[1] = 0xFF;
        if (x>=1196) data[2] = 0xFF;
        for (int c=0; c<3; c++) {
            for (int y=0; y<4; y++) {
                lines[y][x][c] =data[c];
            }
            for (int y=4; y<8; y++) {
                lines[y][(x+896)%1792][c] =data[c];
            }
        }
    }
}




