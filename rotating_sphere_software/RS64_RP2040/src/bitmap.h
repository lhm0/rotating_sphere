// =========================================================================================================================================
//                                                 Rotating Display RS64c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


// ******************************************************************************************************************************************
//
//          this class generates a bitmap for the display. It provides methods for generating clock screens,
//          as well as printing text on the bitmap. The bitmap shall be handed over to the RS64 object. 
//          
// ******************************************************************************************************************************************

// bmp[][][] is defined in globals.h, in order to move the array to static memory
// bmp[x][y]is a representation of the 256x64 pixels. The matrix stores the color number. 0 means "pixel not in use"

#ifndef bitmap_H
#define bitmap_H

#include <Arduino.h>

class bitmap {
  private:

    int8_t _colors[256][3];                  // r, g, b of colors
    int _colorsNr;                           // nr of colors used
    uint8_t _pattern[8][7] = {
      {0,0,0,0,0,0,0},
      {0,0,0,1,0,0,0},
      {0,0,1,0,1,0,0},
      {0,1,0,1,0,1,0},
      {0,1,1,0,1,1,0},
      {0,1,1,1,1,1,0},
      {1,1,1,0,1,1,1},
      {1,1,1,1,1,1,1}
    };   

    void _clrBmp();
    void _clrColors();

    int _setColor(int r, int g, int b);                                        // define new color, get color number in return
    void _setPixel(int x, int y, int c);                                      // set pixel in bitmap[][]
    void _drawBox(int x1, int y1, int x2, int y2, int c);                      // clear Box in bitmap[][]
    void _drawLine(int x1, int y1, int x2, int y2, int c);                      // draw line in bitmap[][]
    void _drawCircle(int radius, int c);                                        // draw circle in bitmap[][]
    void _drawRadius(int n, int rStart, int rEnd, int c);   
    
    int _polarToX(int n, int r);
    int _polarToY(int n, int r);

    void _print_16x24(int p_mode, char s[], int xpos, int ypos, int c);
    void _print_12x18(int p_mode, char s[], int xpos, int ypos, int c);
    void _print_10x15(int p_mode, char s[], int xpos, int ypos, int c);
    void _print_icon_30x20(int p_mode, int i_num, int xpos, int ypos, int c);

  public:
      
    bitmap();                                         // Constructor

    bool clockOn = false;
    
    void generateBMP(int mode);                       // generates bitmap according to mode
    void convertBMPtoLines();
};

#endif
