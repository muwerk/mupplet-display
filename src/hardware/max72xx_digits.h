// max72xx_digits.h - 7/14/16 segment digits controlled by MAX7219 or MAX7221 module driver

#pragma once

#include "max72xx.h"
#include "muwerk.h"
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
     * @param csPin     The chip select pin. (default: D8)
     * @param hDisplays Horizontal number of display units. (default: 1)
     * @param vDisplays Vertical number of display units. (default: 1)
     * @param length    Number of digits per unit (default: 8)
     */
    Max72xxDigits(uint8_t csPin = D8, uint8_t hDisplays = 1, uint8_t vDisplays = 1,
                  uint8_t length = 8)
        : driver(csPin, hDisplays * vDisplays), length(length) {
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
            clearScreen();
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

    /*! Set whether text that is too long for the screen width should
            automatically wrap around to the next line (else clip right).
     * @param w true for wrapping, false for clipping
     */
    void setTextWrap(bool w) {
        wrap = w;
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
                uint8_t *pPtr = outputBuffer;
                for (uint8_t unit = 0; unit < driver.getChainLen(); unit++) {
                    pPtr[0] = Max72XX::digit0 + length - digit - 1;
                    pPtr[1] = bitmap[unit * _width + digit];
                    pPtr += 2;
                }
                driver.sendBlock(outputBuffer, pPtr - outputBuffer);
            }
        }
    }

    /*! Empty the frame buffer
     * @param  color Binary (on or off) color to fill with
     */
    virtual void clearScreen() {
        if (bitmap != nullptr) {
            memset(bitmap, 0, length * driver.getChainLen());
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
            if (strchr(" .,-_=", c)) {
                cursor_x++;
            } else if (c >= '0' && c <= '9') {
                cursor_x++;
            } else if (c >= 'A' && c <= 'Z') {
                cursor_x++;
            } else if (c >= 'a' && c <= 'z') {
                cursor_x++;
            }
            return 1;
        }
        uint8_t index = cursor_y * _width + cursor_x;
        if (c == '.' || c == ',') {
            if (cursor_x == 0) {
                bitmap[index] = B10000000;
                cursor_x++;
            } else {
                bitmap[index - 1] |= B10000000;
            }
        } else if (c == ' ') {
            bitmap[index] = B00000000;
            cursor_x++;
        } else if (c == '-') {
            bitmap[index] = B00000001;
            cursor_x++;
        } else if (c == '_') {
            bitmap[index] = B00001000;
            cursor_x++;
        } else if (c == '=') {
            bitmap[index] = B00001001;
            cursor_x++;
        } else if (c >= '0' && c <= '9') {
            bitmap[index] = pgm_read_byte_near(digitTable7Seg + c - '0');
            cursor_x++;
        } else if (c >= 'A' && c <= 'Z') {
            bitmap[index] = pgm_read_byte_near(charTable7Seg + c - 'A');
            cursor_x++;
        } else if (c >= 'a' && c <= 'z') {
            bitmap[index] = pgm_read_byte_near(charTable7Seg + c - 'a');
            cursor_x++;
        }
        return 1;
    }
};

}  // namespace ustd