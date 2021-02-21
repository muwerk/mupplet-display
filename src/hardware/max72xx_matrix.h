// max72xx_matrix.h - 8x8 led matrix controller by MAX7219 or MAX7221 module driver

#pragma once

#include <Adafruit_GFX.h>
#include "max72xx.h"

namespace ustd {

/*! \brief The MAX72XX Matrix Display Class
 *
 * This class derived from Adafruit's core graphics class provides an implementation of a dot matrix
 * display based on 8x8 led modules driven by a maxim MAX7219 or MAX7221 controller connected over
 * SPI.
 *
 * * See https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf
 * * See https://learn.adafruit.com/adafruit-gfx-graphics-library
 */
class Max72xxMatrix : public Adafruit_GFX {
  private:
    // hardware configuration
    Max72XX driver;

    // runtime - pixel and module logic
    uint8_t hDisplays;
    uint8_t *bitmap;
    uint8_t bitmapSize;
    uint8_t *matrixPosition;
    uint8_t *matrixRotation;
    uint8_t *outputBuffer;

  public:
    /*! Instantiate a Max72xxMatrix instance
     *
     * @param csPin     The chip select pin. (default: D8)
     * @param hDisplays Horizontal number of 8x8 display units. (default: 1)
     * @param vDisplays Vertical number of 8x8 display units. (default: 1)
     * @param rotation  Define if and how the displays are rotated. The first display
     *                  is the one closest to the Connections. `rotation` can be a numeric value
     *                  from 0 to 3 representing respectively no rotation, 90 degrees clockwise, 180
     *                  degrees and 90 degrees counter clockwise.
     */
    Max72xxMatrix(uint8_t csPin = D8, uint8_t hDisplays = 1, uint8_t vDisplays = 1,
                  uint8_t rotation = 0)
        : Adafruit_GFX(hDisplays << 3, vDisplays << 3), driver(csPin, hDisplays * vDisplays),
          hDisplays(hDisplays) {
        uint8_t displays = hDisplays * vDisplays;
        bitmapSize = displays * 8;
        bitmap = (uint8_t *)malloc(bitmapSize + (4 * displays));
        matrixPosition = bitmap + bitmapSize;
        matrixRotation = matrixPosition + displays;
        outputBuffer = matrixRotation + displays;

        for (uint8_t display = 0; display < displays; display++) {
            matrixPosition[display] = display;
            matrixRotation[display] = rotation;
        }
    }

    virtual ~Max72xxMatrix() {
        free(bitmap);
    }

    /*! Start the matrix display
     */
    void begin() {
        if (bitmap != nullptr) {
            // Initialize hardware
            driver.begin();
            driver.setTestMode(false);
            driver.setScanLimit(7);
            driver.setDecodeMode(0);

            // Clear the display
            fillScreen(0);
            write();
        }
    }

    /*! Set the power saving mode for the display
     * @param powersave If `true` the display goes into power-down mode. Set to `false` for normal
     * operation.
     */
    inline void setPowerSave(bool powersave) {
        driver.setPowerSave(powersave);
    }

    /*! Set the brightness of the display
     * @param intensity The brightness of the display. (0..15)
     */
    inline void setIntensity(uint8_t intensity) {
        driver.setIntensity(intensity);
    }

    /*! Set the test mode for the display
     * @param testmode  If `true` the display goes into test mode. Set to `false` for normal
     * operation.
     */
    inline void setTestMode(bool testmode) {
        driver.setTestMode(testmode);
    }

    /*! Flushes the frame buffer to the display
     *
     * In order to implement flicker free double buffering, no graphic function has any immediate
     * effect on the display. All graphic operations are buffered into a frame buffer. By calling
     * this method, the current content of the frame buffer is displayed.
     */
    void write() {
        if (bitmap != nullptr) {
            for (uint8_t opcode = Max72XX::digit7; opcode >= Max72XX::digit0; opcode--) {
                uint16_t endOffset = opcode - Max72XX::digit0;
                uint16_t startOffset = bitmapSize + endOffset;
                uint8_t *pPtr = outputBuffer;
                do {
                    startOffset -= 8;
                    pPtr[0] = opcode;
                    pPtr[1] = bitmap[startOffset];
                    pPtr += 2;
                } while (startOffset > endOffset);
                driver.sendBlock(outputBuffer, bitmapSize / 4);
            }
        }
    }

    /*! Define how the displays are ordered.
     * @param display   Display index (0 is the first)
     * @param x         Horizontal display index
     * @param y         Vertical display index
     */
    void setPosition(uint8_t display, uint8_t x, uint8_t y) {
        if (bitmap != nullptr) {
            matrixPosition[x + hDisplays * y] = display;
        }
    }

    /*! Define if and how the displays are rotated.
     * @param display   Display index (0 is the first)
     * @param rotation  Rotation of the display:
     *   0: no rotation
     *   1: 90 degrees clockwise
     *   2: 180 degrees
     *   3: 90 degrees counter clockwise
     */
    void setRotation(uint8_t display, uint8_t rotation) {
        if (bitmap != nullptr) {
            matrixRotation[display] = rotation;
        }
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

  public:
    // overrides of virtual functions

    /*! Fill the framebuffer completely with one color
     * @param  color Binary (on or off) color to fill with
     */
    virtual void fillScreen(uint16_t color) {
        if (bitmap != nullptr) {
            memset(bitmap, color ? 0xff : 0, bitmapSize);
        }
    }

    /*! Draw a pixel to the canvas framebuffer
     * @param xx    x coordinate
     * @param yy    y coordinate
     * @param color 8-bit Color to fill with. Only lower byte of uint16_t is used.
     */
    void drawPixel(int16_t xx, int16_t yy, uint16_t color) {
        if (bitmap == nullptr) {
            return;
        }

        // Operating in bytes is faster and takes less code to run. We don't
        // need values above 200, so switch from 16 bit ints to 8 bit unsigned
        // ints (bytes).
        // Keep xx as int16_t so fix 16 panel limit
        int16_t x = xx;
        uint8_t y = yy;
        uint8_t tmp;

        if (rotation) {
            // Implement Adafruit's rotation.
            if (rotation >= 2) {
                // rotation == 2 || rotation == 3
                x = _width - 1 - x;
            }

            if (rotation == 1 || rotation == 2) {
                // rotation == 1 || rotation == 2
                y = _height - 1 - y;
            }

            if (rotation & 1) {
                // rotation == 1 || rotation == 3
                tmp = x;
                x = y;
                y = tmp;
            }
        }

        if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
            // Ignore pixels outside the canvas.
            return;
        }

        // Translate the x, y coordinate according to the layout of the
        // displays. They can be ordered and rotated (0, 90, 180, 270).

        uint8_t display = matrixPosition[(x >> 3) + hDisplays * (y >> 3)];
        x &= 0b111;
        y &= 0b111;

        uint8_t r = matrixRotation[display];
        if (r >= 2) {
            // 180 or 270 degrees
            x = 7 - x;
        }
        if (r == 1 || r == 2) {
            // 90 or 180 degrees
            y = 7 - y;
        }
        if (r & 1) {
            // 90 or 270 degrees
            tmp = x;
            x = y;
            y = tmp;
        }

        uint8_t d = display / hDisplays;
        x += (display - d * hDisplays) << 3;  // x += (display % hDisplays) * 8
        y += d << 3;                          // y += (display / hDisplays) * 8

        // Update the color bit in our bitmap buffer.

        uint8_t *ptr = bitmap + x + WIDTH * (y >> 3);
        uint8_t val = 1 << (y & 0b111);

        if (color) {
            *ptr |= val;
        } else {
            *ptr &= ~val;
        }
    }
};
}  // namespace ustd