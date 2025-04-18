// =========================================================================================================================================
//                                                 Rotating Display RS64
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


#include "bitmap.h"
#include "bitmapTemplates.h"
#include "globals.h"

#include <Arduino.h>

int8_t _colors[256][3];
int _colorsNr = 0;

// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

bitmap::bitmap() {
}

// =========================================================================================================================================
//                                                      generateBMP method
// =========================================================================================================================================

void bitmap::generateBMP(int mode) {                            // generates bitmap according to mode
  _clrBmp();
  //_clrColors();

  char text[64];

  int c = _setColor(7,0,0);
  myRP.getTime();
  sprintf(text,":");
  _print_12x18(1,text,113,10, c);
  _print_12x18(1,text,143,10, c);
  sprintf(text,"%02d",myRP.local.tm_hour);
  _print_12x18(1,text,98,10, c);
  sprintf(text,"%02d",myRP.local.tm_min);
  _print_12x18(1,text,128,10, c);
  sprintf(text,"%02d",myRP.local.tm_sec);
  _print_12x18(1,text,158,10, c);

}

// =========================================================================================================================================
//                                                      clr methods
// =========================================================================================================================================

void bitmap::_clrBmp() {
  for (int x=0; x<256; x++) {
    for (int y=0; y<64; y++) {
      bmp[x][y]=0x00;
    }
  }
}

void bitmap::_clrColors() {
  _colorsNr = 0;                  // reset nr of colors used
}

// =========================================================================================================================================
//                        methods for setting bitmap pixels and drawing  on bitmap
// =========================================================================================================================================

// define new color, get color number in return
int bitmap::_setColor(int r, int g, int b) {    
  // check if color has been used already
  for (int c=0; c<_colorsNr; c++) {
    if ((_colors[c][0]==r)&&(_colors[c][1]==g)&&(_colors[c][2]==b)) {
      return c;
    }
  }
  
  // assign next free nr. Position 0 will not be used. 0 means "pixel not used"
  if (_colorsNr<254) {
    _colorsNr++;
    _colors[_colorsNr][0]=(uint8_t)r;
    _colors[_colorsNr][1]=(uint8_t)g;
    _colors[_colorsNr][2]=(uint8_t)b;
  }
  return _colorsNr;
}                                        

// set pixel in bmp[x][y] to color number. If bmp[][]==0, pixel not used and will be transparent.
// x and y will be re-organized, such that the matrix corresponds to the LED sequence and position:
//
//  y=63 -> LED=31  x = x
//  y=62 -> LED=63  x = (x+128)%256
//  ....
//  y=05 -> LED=02  x = x
//  y=04 -> LED=34  x = (x+128)%256
//  y=03 -> LED=01  x = x
//  y=02 -> LED=33  x = (x+128)%256
//  y=01 -> LED=00  x = x
//  y=00 -> LED=32  x = (x+128)%256
//
void bitmap::_setPixel(int x, int y, int c) {
  y = 63-y;                                 // flip upside down
  x = 255-x;                                // flip left to right
  int yn = ((y+1)%2)*32+(y/2);              // convert x and y as described above
  int xn = x;
  if (y%2 == 0) {
    xn = (x+128)%256;
  }
  bmp[xn][yn]=(uint8_t)c;
}

void bitmap::_drawBox(int x1, int y1, int x2, int y2, int c) {
  if (x1 > x2) {
    int aux = x1;
    x1 = x2;
    x2 = aux;
  }
  if (y1 > y2) {
    int aux = y2;
    y1 = y2;
    y2 = aux;
  }
  for (int x=x1; x<x2; x++) {
    for (int y=y1; y<y2; y++) _setPixel(x, y, c);
  }
}

void bitmap::_drawLine(int x1, int y1, int x2, int y2, int c) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    _setPixel(x1, y1, c);

    if (x1 == x2 && y1 == y2)
    break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}

// draw Circle. Center is x = 128, y = 32
void bitmap::_drawCircle(int radius, int c) {
  int rr = radius;
  for (int n=0; n<240; n++) _setPixel(_polarToX(n, rr), _polarToY(n, rr), c);
}

// draw Radius. Angle n in degrees (0...360)
void bitmap::_drawRadius(int n, int rStart, int rEnd, int c) {
  for (int rr=rStart; rr<rEnd; rr++) {
    int x = _polarToX(n, rr);
    int y = _polarToY(n, rr);
    _setPixel(x, y, c);
  }
}

int bitmap::_polarToX(int n, int r) {
  float theta = 0.017453293*(n+0.5);                // 2*pi/360 = 0.017453293
  return (int)roundf(128+r*sin(theta));
}

int bitmap::_polarToY(int n, int r) {
  float theta = 0.017453293*(n+0.5);                // 2*pi/360 = 0.017453293
  return (int)roundf(32+r*cos(theta));
}


// =========================================================================================================================================
//                                  methods for printing characters, numbers, and weather icons on bitmap
// =========================================================================================================================================

void bitmap::_print_16x24(int p_mode, char s[], int xpos, int ypos, int c) {             // p_mode = print mode
  int l,j,x;                                                                      // p_mode = 0 => right
  unsigned long chr;                                                              // p_mode = 1 => mid
  l= strlen(s);                                                                   // p_mode = 2 => left

  if (p_mode==1) {
    xpos=xpos-9*l;
  }
  if (p_mode==2) {
    xpos=xpos-18*l;
  }

  for (j=0; j<l; j++) {                                                           // char #
    for (x=0; x<16; x++) {                                                        // x pixel
      memcpy(&chr, &chr_16x24[s[j]-0x30][3*x], 3);         // copy 3 bytes to chr
      for (int y = 0; y<24; y++) {
        if ((chr & 0x000001) == 1) _setPixel(xpos+j*16+x, ypos+y, c);
        chr>>=1;
      }
    }
  }
}

void bitmap::_print_12x18(int p_mode, char s[], int xpos, int ypos, int c) {                      // p_mode = print mode
  int l,j,x;                                                                      // p_mode = 0 => right
  unsigned long chr;                                                              // p_mode = 1 => mid
  l= strlen(s);                                                                   // p_mode = 2 => left

  if (p_mode==1) {
    xpos=xpos-6*l;
  }
  if (p_mode==2) {
    xpos=xpos-12*l;
  }

  for (j=0; j<l; j++) {                                                           // char #
    for (x=0; x<12; x++) {                                                        // x pixel
      memcpy(&chr, &chr_12x18[s[j]-0x20][3*x], 3);         // copy 3 bytes to chr
      for (int y = 0; y<18; y++) {
        if ((chr & 0x000001) == 1) _setPixel(xpos+j*12+x, ypos+y, c);
        chr>>=1;
      }
    }
  }
}

void bitmap::_print_10x15(int p_mode, char s[], int xpos, int ypos, int c) {                      // p_mode = print mode
  int l,j,x;                                                                      // p_mode = 0 => right
  unsigned long chr;                                                              // p_mode = 1 => mid
  l= strlen(s);                                                                   // p_mode = 2 => left

  if (p_mode==1) {
    xpos=xpos-5*l;
  }
  if (p_mode==2) {
    xpos=xpos-10*l;
  }

  for (j=0; j<l; j++) {                                                           // char #
    for (x=0; x<10; x++) {                                                        // x pixel
      memcpy(&chr, &chr_10x15[s[j]-0x20][2*x], 2);         // copy 2 bytes to chr
      for (int y = 0; y<15; y++) {
        if ((chr & 1) != 0) _setPixel(xpos+j*10+x, ypos+y, c);
        chr>>=1;
      }
    }
  }

}

void bitmap::_print_icon_30x20(int p_mode, int i_num, int xpos, int ypos, int c) {
  int k,x;
  unsigned char chr;
  unsigned long long chr_l=0;

  if (p_mode==1) {
    xpos=xpos-15;
  }
  if (p_mode==2) {
    xpos=xpos-30;
  }

  for (x=0; x<30; x++) {
    chr_l=0;
    for (k=0; k<3; k++) {
      chr_l<<=8;
      chr=(icons_30x20[i_num][x][2-k]);
      chr_l += chr;
    }
    for (int y = 0; y<20; y++) {
      if ((chr_l & 1) != 0) _setPixel(xpos+x, ypos+y, c);
      chr_l>>=1;
    }
  }
}

// convert bitmap to lines pattern. Function generates 7 lines for each pixel line.
// bmpLines[LED_set][][]. LED_sets are 0 = LED00, LED01, ... LED07; 1 = LED08, ..., LED15, ...
// bmpTemplateLines[][][] indicates, whether a pixel is used.
// LED output = (lines[][][] & bmpTemplateLines[][][]) | bmpLines[][][];
//
void bitmap::convertBMPtoLines() {
  for (int x=0; x<256; x++) {
    for (int byte=0; byte<8; byte++) {
      uint8_t temp[8];                    // template bits (same for all sublines and colors)
      uint8_t dat[8][7][3];               // LED data for 8 bits, 7 sublines, 3 colors
      for (int bit=0; bit<8; bit++) {
        int y = 8*(byte)+(bit);
        uint8_t c = bmp[x][y];
        if (c==0) {                       // if color = 0 ==> template bit = 1 ==> will not be affected
          temp[bit] = 1;
        } else {
          temp[bit] = 0;
        }     
        for (int subline=0; subline<7; subline++) {
          for (int i=0; i<3; i++) {
            dat[bit][subline][i]=_pattern[_colors[c][i]][subline];
          }
        }
      }
      uint8_t t = 0;
      for (int bit=0; bit<8; bit++) {
        t = (t<<1)|temp[7-bit];
      }
      for (int subline=0; subline<7; subline++) {
        int l = 7*x+subline;
        bmpTemplateLines[7-byte][l]=t;
        for (int c = 0; c<3; c++) {
          uint8_t led = 0;
          for (int bit=0; bit<8; bit++) {
            led = (led<<1)|dat[7-bit][subline][c];
          }
          bmpLines[7-byte][l][c] = led;
        }
      }
    }
  }
}