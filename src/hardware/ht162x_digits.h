// ht162x_digits.h - 7/16 segment digits controlled by HT1621 (7 seg) or HT1622 (16 seg)

#pragma once


namespace ustd {



/* leo stuff:
static const uint8_t digitTable7Seg[] PROGMEM = {B01111110, B00110000, B01101101, B01111001,
                                                 B00110011, B01011011, B01011111, B01110000,
                                                 B01111111, B01111011};

static const uint8_t charTable7Seg[] PROGMEM = {
    B01110111, B00011111, B00001101, B00111101, B01001111, B01000111, B01011110,
    B00110111, B00000110, B00111100, B00000111, B00001110, B01110110, B00010101,
    B00011101, B01100111, B11101110, B00000101, B01011011, B01000110, B00111110,
    B00011100, B00011100, B01001001, B00100111, B01101101};
*/
/*! \brief The HT162X Digits Display Class
 *
 * This class derived from `Print` class provides an implementation of a 7/16 segment digits display.
 */
class Ht162xDigits : public Print {
  private:
    static const uint8_t digitTable7Seg[]={0b10111110, 0b00000110, 0b01111100, 0b01011110, 0b11000110, 
                        0b11011010, 0b11111010, 0b00001110, 0b11111110, 0b11011110};

    static const uint8_t charTable7Seg[]={
            // A
            0b11101110, 0b11110010, 0b01110000, 0b01110110, 0b11111000, 0b11101000,
            // G
            0b10111010, 0b11100110, 0b00000010, 0b00110110, 0b11100101, 0b10110000,
            // M
            0b10101110, 0b01100010, 0b01110010, 0b11101100, 0b11001111, 0b01100000,
            // S
            0b11011010, 0b11110000, 0b10110110, 0b00110010, 0b10110111, 0b11100110,
            //Y
            0b11100100, 0b01111100 }; 
    // See command summary, datasheet p.13:
    //                                   cmd prefix   command      comment
    //                                          ---   -----------  ------------
    static const uint8_t cmdBIAS    = 0x52;  // 100 | 0 010a bXcX, ab=10 4 commons option, c=1 1/3 cmdcmdBIAS option 
    static const uint8_t cmdSYS_DIS = 0X00;  // 100 | 0 0000 000X, Turn off both system oscillator and LCD bias generator
    static const uint8_t cmdSYS_EN  = 0X02;  // 100 | 0 0000 001X, Turn on system oscillator
    static const uint8_t cmdLCD_OFF = 0X04;  // 100 | 0 0000 010X, Turn off LCD bias generator
    static const uint8_t cmdLCD_ON  = 0X06;  // 100 | 0 0000 011X, Turn on LCD bias generator
    static const uint8_t cmdWDT_DIS = 0X0A;  // 100 | 0 0000 101X, Disable WDT time-out flag output
    static const uint8_t cmdRC_256K = 0X30;  // 100 | 0 0011 0XXX, System clock source, on-chip RC oscillator

    // device configuration
    LcdType lcdType;
    uint8_t csPin;
    uint8_t wrPin;
    uint8_t dataPin;
    uint8_t lcdBacklightPin;
    uint8_t pwmIndexEsp32;

    int digitCnt;
    int digitRawCnt;
    int segmentCnt;
    bool isActive;

    static const long clearDelayMs = 2;
    static const long printDelayMs = 2;
    static const long writeDelayUs = 4;

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
    enum LcdType {lcd12digit_7segment, lcd10digit_16segment};
    static const uint8_t frameBufferSize=12; // 12 positions of 8bit used for HT1621, 11 positions of 16bit used for HT1622 
    uint16_t frameBuffer[frameBufferSize];  // cache for segment state: only rewrite segments on content change

    Ht162xDigits(LcdType lcdType, uint8_t csPin, uint8_t wrPin, uint8_t dataPin, uint8_t lcdBacklightPin=-1, uint8_t pwmIndexEsp32=-1)
        : name(name), lcdType(lcdType), csPin(csPin), wrPin(wrPin), dataPin(dataPin), lcdBacklightPin(lcdBacklightPin), pwmIndexEsp32(pwmIndexEsp32) {
    /*! Instantiates a DisplayDigitsHT162x mupplet
     *
     * No hardware interaction is performed, until \ref begin() is called.
     *
     * @param name Name of the lcd display, used to reference it by pub/sub messages
     * @param lcdType \ref LcdType 
     */
        switch (lcdType) {
            case LcdType::lcd12digit_7segment:
                digitCnt=12; 
                digitRawCnt=13;  // Chinese phone-booth title snips at display top
                segmentCnt=8;
                isActive=true;
                break;
            case LcdType::lcd10digit_16segment:
                digitCnt=10; 
                digitRawCnt=12;  // Special case last digits encodes 9 decimal dots  
                segmentCnt=17;
                isActive=true;
                break;
            default:
                isActive=false;
                break;
        }
    }

    /*! Start the digits display
     */
    void begin() {
        if (isActive) {
            pinMode(csPin, OUTPUT);
            pinMode(wrPin, OUTPUT);
            pinMode(dataPin, OUTPUT);
            if (lcdBacklightPin!=-1) pinMode(lcdBacklightPin, OUTPUT);

            switch (lcdType) {
                case LcdType::lcd12digit_7segment:
                    writeCmd(cmdBIAS);     // set LCD bias
                    writeCmd(cmdRC_256K);  // use internal clock
                    writeCmd(cmdSYS_DIS);  // disable all generators
                    writeCmd(cmdWDT_DIS);  // disable watchdog timer output bitj
                    writeCmd(cmdSYS_EN);   // enable generators
                    setDisplay(true);       // switch on display
                    clear();                // Clear display
                    break;
                case LcdType::lcd10digit_16segment:
                    writeCmd(cmdBIAS);     // set LCD bias
                    writeCmd(cmdRC_256K);  // use internal clock
                    writeCmd(cmdSYS_DIS);  // disable all generators
                    writeCmd(cmdWDT_DIS);  // disable watchdog timer output bitj
                    writeCmd(cmdSYS_EN);   // enable generators
                    setDisplay(true);       // switch on display
                    clear();                // Clear display
                    break;
                default:
                    isActive=false;
                    break;
            }
        }
        return isActive;
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