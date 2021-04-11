// display_matrix_max72xx.h - mupplet for Matrix Display using MAX72xx

#pragma once

#include "muwerk.h"
#include "helper/light_controller.h"
#include "helper/mup_gfx_display.h"
#include "hardware/max72xx_matrix.h"

namespace ustd {

// clang-format off
/*! \brief mupplet-display MAX7219/MAX7221 Led Matrix Display class

The DisplayMatrixMAX72XX mupplet allows to control a led matrix display based on multiple 8x8 led
matrix modules driven by a MAX7219 or MAX7221 connected via SPI.

The mupplet acts as an intelligent display server supporting various commands and scenarios.

## Sample mupplet Integration

\code{cpp}
#define __ESP__ 1   // Platform defines required, see ustd library doc, mainpage.
#include "scheduler.h"
#include "display_matrix_max72xx.h"

ustd::Scheduler sched;
ustd::DisplayMatrixMAX72XX matrix("matrix", D8, 12, 1, 1);

void setup() {
    matrix.begin(&sched);
}
\endcode

More information:
<a href="https://github.com/muwerk/mupplet-display/blob/master/extras/display-matrix-notes.md">DisplayMatrixMAX72XX
Application Notes</a>
*/
// clang-format on

class DisplayMatrixMAX72XX : public MuppletGfxDisplay {
  public:
    static const char *version;  // = "0.1.0";

  private:
    // hardware configuration
    Max72xxMatrix display;

    // runtime
    LightController light;

  public:
    /*! Instantiates a DisplayMatrixMAX72XX mupplet
     *
     * No hardware interaction is performed, until \ref begin() is called.
     *
     * @param name      Name of the display, used to reference it by pub/sub messages
     * @param csPin     The chip select pin.
     * @param hDisplays Horizontal number of 8x8 display units. (default: 1)
     * @param vDisplays Vertical number of 8x8 display units. (default: 1)
     * @param rotation  Define if and how the displays are rotated. The first display
     *                  is the one closest to the Connections. `rotation` can be a numeric value
     *                  from 0 to 3 representing respectively no rotation, 90 degrees clockwise, 180
     *                  degrees and 90 degrees counter clockwise.
     */
    DisplayMatrixMAX72XX(String name, uint8_t csPin, uint8_t hDisplays = 1, uint8_t vDisplays = 1,
                         uint8_t rotation = 0)
        : MuppletGfxDisplay(name, MUPDISP_FEATURE_MONO),
          display(csPin, hDisplays, vDisplays, rotation) {
    }

    /*! Initialize the display hardware and start operation
     * @param _pSched       Pointer to a muwerk scheduler object, used to create worker
     *                      tasks and for message pub/sub.
     * @param initialState  Initial logical state of the display: false=off, true=on.
     */
    void begin(Scheduler *_pSched, bool initialState = false) {
        pSched = _pSched;
        tID = pSched->add([this]() { this->loop(); }, name, 10000L);

        pSched->subscribe(tID, name + "/display/#", [this](String topic, String msg, String orig) {
            this->commandParser(topic.substring(name.length() + 9), msg, name + "/display");
        });
        pSched->subscribe(tID, name + "/light/#", [this](String topic, String msg, String orig) {
            this->light.commandParser(topic.substring(name.length() + 7), msg);
        });

        // initialize default values
        current_font = 0;
#ifdef USTD_FEATURE_PROGRAMPLAYER
        programInit();
#endif
        // prepare hardware
        display.begin();
        display.setTextWrap(false);

        // start light controller
        light.begin([this](bool state, double level, bool control,
                           bool notify) { this->onLightControl(state, level, control, notify); },
                    initialState);
    }

  private:
    void loop() {
        light.loop();
#ifdef USTD_FEATURE_PROGRAMPLAYER
        programLoop();
#endif
    }

    void onLightControl(bool state, double level, bool control, bool notify) {
        uint8_t intensity = (level * 15000) / 1000;
        if (control) {
            display.setIntensity(intensity);
        }
        if (control) {
            display.setPowerSave(!state);
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
        display.fillRect(x, y, w, h, 0);
        display.write();
    }

    virtual void displayClear(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bg) {
        display.fillRect(x, y, w, h, bg);
        display.write();
    }

    virtual void displayPrint(String content, bool ln = false) {
        if (ln) {
            display.println(content);
        } else {
            display.print(content);
        }
        display.write();
    }

    virtual bool displayFormat(int16_t x, int16_t y, int16_t w, int16_t align, String content,
                               uint8_t font, uint16_t color, uint16_t bg) {
        display.setFont(fonts[font]);
        display.setTextColor(color, bg);
        bool ret = display.printFormatted(x, y, w, align, content, sizes[font].baseLine,
                                          sizes[font].yAdvance);
        display.write();
        return ret;
    }

    // implementation
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

const char *DisplayMatrixMAX72XX::version = "0.1.0";

}  // namespace ustd