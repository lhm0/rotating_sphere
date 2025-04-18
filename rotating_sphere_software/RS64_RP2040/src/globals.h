#ifndef GLOBALS_H
#define GLOBALS_H

#include "playlist.h"
#include "my_RP.h"
#include "bitmap.h"
#include "LED_control.h"

extern Playlist playlist;
extern SdFs SD;
extern my_RP myRP;
extern bitmap BMP;
extern LED_control LEDs;

extern uint8_t bmp[256][64];
extern uint8_t bmpLines[8][1792][3];									  
extern uint8_t bmpTemplateLines[8][1792];									 

#endif
