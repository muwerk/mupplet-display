// max72xx_digits.h - 7/14/16 segment digits controlled by MAX7219 or MAX7221 module driver

#pragma once

#include "max72xx.h"
#include "mupplet_core.h"

namespace ustd {

static const uint8_t digitTable7Seg[] PROGMEM = {B01111110, B00110000, B01101101, B01111001,
                                                 B00110011, B01011011, B01011111, B01110000,
                                                 B01111111, B01111011};

static const uint8_t charTable7Seg[] PROGMEM = {
    B01110111, B00011111, B00001101, B00111101, B01001111, B01000111, B01011110,
    B00110111, B00000110, B00111100, B00000111, B00001110, B01110110, B01110110,
    B00011101, B01100111, B11111110, B00000101, B01011011, B01110000, B00111110,
    B00011100, B00011100, B00001001, B00100111, B01101101};

/*! \brief The MAX72XX Digits Display Class
 *
 * This class derived from `Print` class provides an implementation of a 7 segment digits display
 * based on 8 digits modules driven by a maxim MAX7219 or MAX7221 controller connected over
 * SPI.
 *
 * * See https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf
 */
class Max72xxDigits : public Print {
  private:
    // hardware configuration
    Max72XX driver;

    // runtime - pixel and module logic
    uint8_t length;
    uint8_t bitmapSize;
    uint8_t *bitmap;
    uint8_t *outputBuffer;
    int16_t _width;
    int16_t _height;
    int16_t cursor_x;
    int16_t cursor_y;
    bool wrap;

  public:
    /*! Instantiate a Max72xxDigits instance
     *
     * @param csPin     The chip select pin.
     * @param hDisplays Horizontal number of display units. (default: 1)
     * @param vDisplays Vertical number of display units. (default: 1)
     * @param length    Number of digits per unit (default: 8)
     */
    Max72xxDigits(uint8_t csPin, uint8_t hDisplays = 1, uint8_t vDisplays = 1, uint8_t length = 8)
        : driver(csPin, hDisplays * vDisplays), length(length > 8 ? 8 : length) {
        bitmapSize = hDisplays * vDisplays * length;
        bitmap = (uint8_t *)malloc(bitmapSize + (hDisplays * vDisplays * 2));
        outputBuffer = bitmap + bitmapSize;
        _width = hDisplays * length;
        _height = vDisplays;
        cursor_x = 0;
        cursor_y = 0;
        wrap = true;
    }

    /*! Start the digits display
     */
    void begin() {
        if (bitmap != nullptr) {
            // Initialize hardware
            driver.begin();
            driver.setTestMode(false);
            driver.setScanLimit(length);
            driver.setDecodeMode(B00000000);

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

    /*! Set text cursor location
     * @param x X coordinate in digit positions
     * @param y Y coordinate in digit positions
     */
    void setCursor(int16_t x, int16_t y) {
        cursor_x = x;
        cursor_y = y;
    }

    /*! Set text cursor X location
     * @param x X coordinate in digit positions
     */
    inline void setCursorX(int16_t x) {
        cursor_x = x;
    }

    /*! Set text cursor Y location
     * @param y Y coordinate in digit positions
     */
    inline void setCursorY(int16_t y) {
        cursor_y = y;
    }

    /*! Set whether text that is too long for the screen width should
     *      automatically wrap around to the next line (else clip right).
     * @param w true for wrapping, false for clipping
     */
    void setTextWrap(bool w) {
        wrap = w;
    }

    /*! Fill a rectangle completely with one pattern.
     *
     * @param x         Top left corner x coordinate
     * @param y         Top left corner y coordinate
     * @param w         Width in digit positions
     * @param h         Height in digit positions
     * @param pattern   Pattern to fill the area with
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t pattern = B00000000) {
        x = x < 0 ? 0 : x >= _width ? _width - 1 : x;
        y = y < 0 ? 0 : y >= _height ? _height - 1 : y;
        w = w < 0 ? 0 : w >= _width - x ? _width - x : w;
        h = h < 0 ? 0 : h >= _height - y ? _height - y : h;

        for (int16_t yy = y; yy < y + h; yy++) {
            memset(bitmap + yy * _width + x, pattern, w);
        }
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
     * @return          `true` if the string fits the defined space, `false` if output was truncated
     */
    bool printFormatted(int16_t x, int16_t y, int16_t w, int16_t align, String content) {
        uint8_t shadowBuffer[8];
        x = x < 0 ? 0 : x >= _width ? _width - 1 : x;
        y = y < 0 ? 0 : y >= _height ? _height - 1 : y;
        w = w < 0 ? 0 : w >= _width - x ? _width - x : w;
        memset(bitmap + y * _width + x, B00000000, w);
        uint8_t *pDst = shadowBuffer;
        const unsigned char *pSrc = (unsigned char *)content.c_str();
        while (pDst < shadowBuffer + 8 && *pSrc) {
            if (*pSrc < 32) {
                pDst--;
            } else if (*pSrc == '.' || *pSrc == ',') {
                if (pDst == shadowBuffer) {
                    *pDst = B10000000;
                } else {
                    pDst--;
                    *pDst |= B10000000;
                }
            } else if (*pSrc == ' ') {
                *pDst = B00000000;
            } else if (*pSrc == '-') {
                *pDst = B00000001;
            } else if (*pSrc == '_') {
                *pDst = B00001000;
            } else if (*pSrc == '=') {
                *pDst = B00001001;
            } else if (*pSrc >= '0' && *pSrc <= '9') {
                *pDst = pgm_read_byte_near(digitTable7Seg + *pSrc - '0');
            } else if (*pSrc >= 'A' && *pSrc <= 'Z') {
                *pDst = pgm_read_byte_near(charTable7Seg + *pSrc - 'A');
            } else if (*pSrc >= 'a' && *pSrc <= 'z') {
                *pDst = pgm_read_byte_near(charTable7Seg + *pSrc - 'a');
            } else {
                *pDst = B00001000;
            }
            pSrc++;
            pDst++;
        }
        int16_t size = pDst - shadowBuffer;
        int16_t offs = 0;
        pDst = bitmap + (y * length) + x;  // fist position of the destination slot
        switch (align) {
        default:
        case 0:
            // left
            memcpy(pDst, shadowBuffer, min(w, size));
            break;
        case 1:
            // center
            if (w < size) {
                // string is larger than slot size - display only middle part
                offs = (size - w) / 2;
                memcpy(pDst, shadowBuffer + offs, w);
            } else {
                // string is smaller than slot size - display center aligned
                offs = (w - size) / 2;
                memcpy(pDst + offs, shadowBuffer, size);
            }
            break;
        case 2:
            // right
            if (w < size) {
                // string is larger than slot size - display only last part
                offs = size - w;
                memcpy(pDst, shadowBuffer + offs, w);
            } else {
                // string is smaller than slot size - display right aligned
                offs = w - size;
                memcpy(pDst + offs, shadowBuffer, size);
            }
            break;
        }
        return *pSrc == 0 && size <= w;
    }

    /*! Flushes the frame buffer to the display
     *
     * In order to implement flicker free double buffering, no display function has any immediate
     * effect on the display. All display operations are buffered into a frame buffer. By calling
     * this method, the current content of the frame buffer is displayed.
     */
    void write() {
        if (bitmap != nullptr) {
            for (uint8_t digit = 0; digit < length; digit++) {
                uint16_t endOffset = digit;
                uint16_t startOffset = bitmapSize + endOffset;
                uint8_t *pPtr = outputBuffer;
                do {
                    startOffset -= length;
                    pPtr[0] = Max72XX::digit0 + length - digit - 1;
                    pPtr[1] = bitmap[startOffset];
                    pPtr += 2;
                } while (startOffset > endOffset);
                driver.sendBlock(outputBuffer, pPtr - outputBuffer);
            }
        }
    }

    /*! Empty the frame buffer
     * @param pattern Binary (on or off) pattern to fill with
     */
    virtual void fillScreen(uint8_t pattern) {
        if (bitmap != nullptr) {
            memset(bitmap, pattern, length * driver.getChainLen());
            cursor_x = 0;
            cursor_y = 0;
        }
    }

    /*! Get width of the display
     * @returns Width in number of digits
     */
    inline int16_t width() const {
        return _width;
    };

    /*! Get height of the display
     * @returns Height in number of rows
     */
    inline int16_t height() const {
        return _height;
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

    /*! Calculates the length in digits of a char
     * @param c         The ASCII character in question
     * @param firstChar Set to true, if this is the first chat of a sequence (default: false)
     */
    uint8_t getCharLen(unsigned char c, bool firstChar = false) {
        if (c < 32) {
            return 0;
        } else if (c == '.' || c == ',') {
            return firstChar ? 1 : 0;
        }
        return 1;
    }

  protected:
    virtual size_t write(uint8_t c) {
        if (c == '\r') {
            cursor_x = 0;
        } else if (c == '\n') {
            cursor_x = 0;
            cursor_y++;
        }
        if (wrap && cursor_x >= _width) {
            cursor_x = 0;
            cursor_y++;
        }
        if (cursor_x >= _width || cursor_y >= _height) {
            // out of viewport
            return 1;
        }
        if (cursor_x < 0 || cursor_y < 0) {
            // out of viewport but we must increment when char is prinatble
            cursor_x += getCharLen(c, cursor_x == 0);
            return 1;
        }
        uint8_t index = cursor_y * _width + cursor_x;
        if (c < 32) {
            return 1;
        } else if (c == '.' || c == ',') {
            if (cursor_x == 0) {
                bitmap[index] = B10000000;
            } else {
                bitmap[index - 1] |= B10000000;
                cursor_x--;
            }
        } else if (c == ' ') {
            bitmap[index] = B00000000;
        } else if (c == '-') {
            bitmap[index] = B00000001;
        } else if (c == '_') {
            bitmap[index] = B00001000;
        } else if (c == '=') {
            bitmap[index] = B00001001;
        } else if (c >= '0' && c <= '9') {
            bitmap[index] = pgm_read_byte_near(digitTable7Seg + c - '0');
        } else if (c >= 'A' && c <= 'Z') {
            bitmap[index] = pgm_read_byte_near(charTable7Seg + c - 'A');
        } else if (c >= 'a' && c <= 'z') {
            bitmap[index] = pgm_read_byte_near(charTable7Seg + c - 'a');
        } else {
            bitmap[index] = B00001000;
        }
        cursor_x++;
        return 1;
    }
};

}  // namespace ustd