// st7735_matrix.h -  ST7735B and ST7735R TFT module driver

#pragma once

#include "Adafruit_ST7735.h"

namespace ustd {

/*! \brief The ST7735 Matrix Display Class
 *
 * This class derived from Adafruit's ST7735 TFT driver class provides an implementation of a TFT
 * dot matrix display based on the Sitronix ST7735 color single-chip TFT controller connected via
 * SPI.
 *
 * * See https://www.displayfuture.com/Display/datasheet/controller/ST7735.pdf
 * * See https://github.com/adafruit/Adafruit-ST7735-Library
 * * See https://learn.adafruit.com/1-8-tft-display/graphics-library
 * * See https://learn.adafruit.com/adafruit-gfx-graphics-library
 */
class St7735Matrix : public Adafruit_ST7735 {
  protected:
    uint8_t hardware;
    uint8_t rotation;

  public:
    /*! Instantiate Adafruit ST7735 driver with default hardware SPI
     * @param csPin     Chip select pin #
     * @param dcPin     Data/Command pin #
     * @param rsPin     Reset pin # (optional, pass -1 if unused)
     * @param hardware  Hardware type (one of INITR_GREENTAB, INITR_REDTAB, INITR_BLACKTAB,
     *                  INITR_MINI160x80 or INITR_HALLOWING)
     * @param rotation  Define if and how the display is rotated. `rotation` can be a numeric value
     *                  from 0 to 3 representing respectively no rotation, 90 degrees clockwise, 180
     *                  degrees and 90 degrees counter clockwise.
     */
    St7735Matrix(uint8_t csPin, uint8_t dcPin, uint8_t rsPin, uint8_t hardware, uint8_t rotation)
        : Adafruit_ST7735(csPin, dcPin, rsPin), hardware(hardware), rotation(rotation) {
        textbgcolor = ST77XX_BLACK;
        textcolor = ST77XX_WHITE;
    }

    /*! Start the matrix display
     */
    void begin() {
        initR(hardware);
        fillScreen(textbgcolor);
        setRotation(rotation);
    }

    /*! Returns if too long text will be wrapped to the next line
     * @returns Wrapping mode
     */
    inline bool getTextWrap() const {
        return wrap;
    }

    /*! Get text cursor X location
     * @returns X coordinate in digit positions
     */
    inline int16_t getCursorX() const {
        return cursor_x;
    }

    /*! Get text cursor Y location
     * @returns Y coordinate in digit positions
     */
    inline int16_t getCursorY() const {
        return cursor_y;
    };

    /*! Get text color value
     * @returns Text color value
     */
    inline uint16_t getTextColor() const {
        return textcolor;
    }

    /*! Get text background color value
     * @returns Text background color value
     */
    inline uint16_t getTextBackground() const {
        return textbgcolor;
    }

    /*! Calculates the bounding box of a character
     *
     * @param  c     The ASCII character in question
     * @param  x     Pointer to x location of character. Value is modified by
     *               this function to advance to next character.
     * @param  y     Pointer to y location of character. Value is modified by
     *               this function to advance to next character.
     * @param  minx  Pointer to minimum X coordinate, passed in to AND returned
     *               by this function -- this is used to incrementally build a
     *               bounding rectangle for a string.
     * @param  miny  Pointer to minimum Y coord, passed in AND returned.
     * @param  maxx  Pointer to maximum X coord, passed in AND returned.
     * @param  maxy  Pointer to maximum Y coord, passed in AND returned.
     */
    void getCharBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny,
                       int16_t *maxx, int16_t *maxy) {
        charBounds(c, x, y, minx, miny, maxx, maxy);
    }

    /*! Prints a text at a specified location with a specified formatting
     *
     * This method prints a text at the specified location with the specified length using left,
     * right or centered alignment. All parameters are checked for plasibility and will be adapted
     * to the current display size.
     *
     * @param x         Top left corner x coordinate
     * @param y         Top left corner y coordinate
     * @param w         Width in digit positions
     * @param align     Alignment of the string to display: 0 = left, 1 = center, 2 = right
     * @param content   The string to print
     * @param baseLine  The distance between baseline and topline
     * @param yAdvance  The newline distance - If specified, the height of the calculated bounding
     *                  box is adjusted to a multiple of this value
     * @return          `true` if the string fits the defined space, `false` if output was truncated
     */
    bool printFormatted(int16_t x, int16_t y, int16_t w, int16_t align, String content,
                        uint8_t baseLine, uint8_t yAdvance = 0) {
        bool old_wrap = wrap;
        int16_t xx = 0, yy = 0;
        uint16_t ww = 0, hh = 0;
        getTextBounds(content, 0, 0, &xx, &yy, &ww, &hh);
        wrap = old_wrap;

        switch (align) {
        default:
        case 0:
            // left
            xx = 0;
            break;
        case 1:
            // center
            xx = (w - ww) / 2;
            break;
        case 2:
            // right
            xx = w - ww;
            break;
        }
        if (yAdvance && ww % yAdvance) {
            hh = ((hh / yAdvance) + 1) * yAdvance;
        }
        GFXcanvas16 tmp(w, hh);
        if (gfxFont) {
            tmp.setFont(gfxFont);
        }
        tmp.fillScreen(textbgcolor);
        tmp.setTextWrap(false);
        tmp.setCursor(xx, baseLine ? baseLine : -1 * yy);
        tmp.setTextColor(textcolor, textbgcolor);
        tmp.print(content);
        drawRGBBitmap(x, y, tmp.getBuffer(), w, hh);
        // set cursor after last printed character
        setCursor(x + tmp.getCursorX(), y + (baseLine ? baseLine : -1 * yy));
        return w >= (int16_t)ww;
    }
};

}  // namespace ustd