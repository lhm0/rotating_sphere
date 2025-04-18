#include "playlist.h"
#include "my_RP.h"
#include "bitmap.h"
#include "LED_control.h"
#include <LittleFS.h>

Playlist playlist;
SdFs SD;
my_RP myRP;
bitmap BMP;
LED_control LEDs;       

uint8_t bmp[256][64];
uint8_t bmpLines[8][1792][3];									   // 
uint8_t bmpTemplateLines[8][1792];									   // 

