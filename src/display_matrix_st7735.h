// display_matrix_st7735.h - mupplet for ZFZ Matrix Display using ST7735

#pragma once

#include "muwerk.h"
#include "helper/light_controller.h"
#include "helper/mup_gfx_display.h"
#include "hardware/st7735_matrix.h"

namespace ustd {
class DisplayMatrixST7735 : public MuppletGfxDisplay {
  public:
    static const char *version;  // = "0.1.0";

  private:
    // hardware configuration
    St7735Matrix display;
    uint8_t blPin;
    bool blActiveLogic = false;
    uint8_t blChannel;
    uint16_t blPwmRange;

    // runtime
    LightController light;

  public:
    /*! Instantiates a DisplayMatrixST7735 mupplet
     *
     * No hardware interaction is performed, until \ref begin() is called.
     *
     * @param name          Name of the display, used to reference it by pub/sub messages
     * @param hardware      Hardware type (one of INITR_GREENTAB, INITR_REDTAB, INITR_BLACKTAB,
     *                      INITR_MINI160x80 or INITR_HALLOWING)
     * @param rotation      Define if and how the display is rotated. `rotation` can be a numeric
     *                      value from 0 to 3 representing respectively no rotation, 90 degrees
     *                      clockwise, 180 degrees and 90 degrees counter clockwise.
     * @param csPin         The chip select pin #
     * @param dcPin         The data/command pin #
     * @param rsPin         The reset pin # (optional, pass -1 if unused)
     * @param blPin         The back light pin # (optional, pass -1 if no backlight control)
     * @param blActiveLogic Characterizes the physical logicl-level which would turn
     *                      the backlilght on. Default is 'false', which assumes the led
     *                      turns on if logic level at the GPIO port is LOW. Change
     *                      to 'true', if led is turned on by physical logic level HIGH.
     * @param blChannel     Currently ESP32 only, can be ignored for all other platforms.
     *                      ESP32 requires assignment of a system-wide unique channel number
     *                      (0..15) for each led in the system. So for ESP32 both the GPIO port
     *                      number and a unique channel id are required.
     */
    DisplayMatrixST7735(String name, uint8_t hardware, uint8_t rotation, uint8_t csPin,
                        uint8_t dcPin, uint8_t rsPin = -1, uint8_t blPin = -1,
                        bool blActiveLogic = false, uint8_t blChannel = 0)
        : MuppletGfxDisplay(name), display(csPin, dcPin, rsPin, hardware, rotation), blPin(blPin),
          blActiveLogic(blActiveLogic), blChannel(blChannel) {
        current_bg = ST7735_BLACK;
        current_fg = ST7735_WHITE;
    }

    /*! Initialize the display hardware and start operation
     * @param _pSched       Pointer to a muwerk scheduler object, used to create worker
     *                      tasks and for message pub/sub.
     * @param initialState  Initial logical state of the display: false=off, true=on.
     */
    void begin(Scheduler *_pSched, bool initialState = false) {
        pSched = _pSched;
        tID = pSched->add([this]() { this->loop(); }, name, 10000L);

        pSched->subscribe(tID, name + "/display/#", [this](String top, String msg, String org) {
            this->commandParser(top.substring(name.length() + 9), msg, name + "/display");
        });
        if (blPin != -1 && blPin != 0) {
            // backlight control enabled - initialize hardware
            initBacklightHardware();
            // backlight control enabled - register commadn parser
            pSched->subscribe(tID, name + "/light/#", [this](String top, String msg, String org) {
                this->light.commandParser(top.substring(name.length() + 7), msg);
            });
        }

        // initialize default values
        current_font = 0;
#ifdef USTD_FEATURE_PROGRAMPLAYER
        programInit();
#endif

        // prepare hardware
        display.begin();
        display.setTextWrap(false);
        display.setTextColor(current_fg, current_bg);

        // start light controller
        if (blPin != -1 && blPin != 0) {
            light.begin(
                [this](bool state, double level, bool control, bool notify) {
                    this->onLightControl(state, level, control, notify);
                },
                initialState);
        } else {
            display.enableDisplay(true);
        }
    }

  private:
    void loop() {
        light.loop();
#ifdef USTD_FEATURE_PROGRAMPLAYER
        programLoop();
#endif
    }

    void onLightControl(bool state, double level, bool control, bool notify) {
        if (control) {
            if (state && level == 1.0) {
                // backlight is on at maximum brightness
#ifdef __ESP32__
                ledcWrite(blChannel, blActiveLogic ? blPwmRange : 0);
#else
                digitalWrite(blPin, blActiveLogic ? HIGH : LOW);
#endif
            } else if (state && level > 0.0) {
                // backlight is dimmed
                uint16_t bri = (uint16_t)(level * (double)blPwmRange);
                if (bri) {
                    if (!blActiveLogic) {
                        bri = blPwmRange - bri;
                    }
#ifdef __ESP32__
                    ledcWrite(blChannel, bri);
#else
                    analogWrite(blPin, bri);
#endif
                } else {
                    light.forceState(false, 0.0);
                    onLightControl(false, 0.0, control, notify);
                }
            } else {
                // backlight is off
#ifdef __ESP32__
                ledcWrite(blChannel, blActiveLogic ? 0 : blPwmRange);
#else
                digitalWrite(blPin, blActiveLogic ? LOW : HIGH);
#endif
            }
        }
        if (notify) {
            pSched->publish(name + "/light/unitbrightness", String(level, 3));
            pSched->publish(name + "/light/state", state ? "on" : "off");
        }
    }

    // abstract methods implementation
    virtual void getDimensions(int16_t &width, int16_t &height) {
        width = display.width();
        height = display.height();
    }

    virtual bool getTextWrap() {
        return display.getTextWrap();
    }

    virtual void setTextWrap(bool wrap) {
        display.setTextWrap(wrap);
    }

    virtual void setTextFont(uint8_t font, int16_t baseLineAdjustment) {
        display.setFont(fonts[font]);
        if (baseLineAdjustment) {
            int16_t y = display.getCursorY();
            display.setCursor(display.getCursorX(), y + baseLineAdjustment);
        }
    }

    virtual void getCursor(int16_t &x, int16_t &y) {
        x = display.getCursorX();
        y = display.getCursorY();
    }

    virtual void setCursor(int16_t x, int16_t y) {
        display.setCursor(x, y);
    }

    virtual void displayClear(int16_t x, int16_t y, int16_t w, int16_t h) {
        display.fillRect(x, y, w, h, display.getTextBackground());
    }

    virtual void displayClear(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bg) {
        display.fillRect(x, y, w, h, bg);
    }

    virtual void displayPrint(String content, bool ln = false) {
        if (ln) {
            display.println(content);
        } else {
            display.print(content);
        }
    }

    virtual bool displayFormat(int16_t x, int16_t y, int16_t w, int16_t align, String content,
                               uint8_t font, uint16_t color, uint16_t bg) {
        display.setFont(fonts[font]);
        display.setTextColor(color, bg);
        return display.printFormatted(x, y, w, align, content, sizes[font].baseLine,
                                      sizes[font].yAdvance);
    }

#ifdef USTD_FEATURE_PROGRAMPLAYER
    virtual bool initNextCharDimensions(ProgramItem &item) {
        while (charPos < item.content.length()) {
            int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1;
            int16_t x = 0, y = sizes[item.font].baseLine;
            display.getCharBounds(item.content[charPos], &x, &y, &minx, &miny, &maxx, &maxy);
            if (maxx >= minx) {
                charX = x;
                charY = sizes[item.font].yAdvance;
                if (item.content[charPos] == ' ') {
                    lastPos += charX;
                } else {
                    return true;
                }
            } else if (item.content[charPos] == ' ') {
                lastPos += charX;
            }
            // char is not printable
            ++charPos;
        }
        // end of string
        return false;
    }
#endif

    // implementation
    void initBacklightHardware() {
#if defined(__ESP32__)
        pinMode(blPin, OUTPUT);
// use first channel of 16 channels (started from zero)
#define LEDC_TIMER_BITS 10
// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000
        ledcSetup(blChannel, LEDC_BASE_FREQ, LEDC_TIMER_BITS);
        ledcAttachPin(blPin, blChannel);
#else
        pinMode(blPin, OUTPUT);
#endif
#ifdef __ESP__
        blPwmRange = 1023;
#else
        blPwmRange = 255;
#endif
    }

    void getTextDimensions(uint8_t font, const char *content, int16_t &width, int16_t &height) {
        if (!content || !*content) {
            width = 0;
            height = 0;
            return;
        }
        int16_t x, y;
        uint16_t w, h;
        uint8_t old_font = current_font;
        bool old_wrap = display.getTextWrap();
        display.setFont(fonts[font]);
        display.setTextWrap(false);
        display.getTextBounds(content, 0, sizes[font].baseLine, &x, &y, &w, &h);
        display.setTextWrap(old_wrap);
        display.setFont(fonts[old_font]);
        width = (int16_t)w;
        height = (int16_t)h;
    }
};

}  // namespace ustd