// display_digits_max72xx.h - mupplet for 7 segment digits display using MAX72xx

#pragma once

#include "ustd_array.h"
#include "muwerk.h"
#include "helper/light_controller.h"
#include "helper/mup_display.h"
#include "hardware/max72xx_digits.h"

namespace ustd {

class DisplayDigitsMAX72XX : public MuppletDisplay {
  public:
    static const char *version;  // = "0.1.0";

  private:
    // hardware configuration
    Max72xxDigits max;

    // runtime
    LightController light;

  public:
    /*! Instantiates a DisplayMatrixMAX72XX mupplet
     *
     * No hardware interaction is performed, until \ref begin() is called.
     *
     * @param name Name of the led, used to reference it by pub/sub messages
     * @param csPin     The chip select pin. (default: D8)
     * @param hDisplays Horizontal number of display units. (default: 1)
     * @param vDisplays Vertical number of display units. (default: 1)
     * @param length    Number of digits per unit (default: 8)
     */
    DisplayDigitsMAX72XX(String name, uint8_t csPin = D8, uint8_t hDisplays = 1,
                         uint8_t vDisplays = 1, uint8_t length = 8)
        : MuppletDisplay(name), max(csPin, hDisplays, vDisplays, length) {
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

        // prepare hardware
        max.begin();
        max.setTextWrap(false);

        // start light controller
        light.begin([this](bool state, double level, bool control,
                           bool notify) { this->onLightControl(state, level, control, notify); },
                    initialState);
    }

  private:
    void loop() {
        light.loop();
    }

    void onLightControl(bool state, double level, bool control, bool notify) {
        uint8_t intensity = (level * 15000) / 1000;
        if (control) {
            max.setIntensity(intensity);
        }
        if (control) {
            max.setPowerSave(!state);
        }
        if (notify) {
            pSched->publish(name + "/light/unitbrightness", String(level, 3));
            pSched->publish(name + "/light/state", state ? "on" : "off");
        }
    }

    virtual void getDimensions(int16_t &width, int16_t &height) {
        width = max.width();
        height = max.height();
    }

    virtual bool getTextWrap() {
        return max.getTextWrap();
    }

    virtual void setTextWrap(bool wrap) {
        max.setTextWrap(wrap);
    }

    virtual uint8_t getTextFont() {
        return 0;
    }

    virtual FontSize getTextFontSize() {
        FontSize retVal;
        retVal.baseLine = 0;
        retVal.xAdvance = 1;
        retVal.yAdvance = 1;
        return retVal;
    }

    virtual void setTextFont(uint8_t font) {
    }

    virtual void getCursor(int16_t &x, int16_t &y) {
        x = max.getCursorX();
        y = max.getCursorY();
    }

    virtual void setCursor(int16_t x, int16_t y) {
        max.setCursor(x, y);
    }

    virtual void displayClear(int16_t x, int16_t y, int16_t w, int16_t h, bool flush = true) {
        max.fillRect(x, y, w, h);
        if (flush) {
            max.write();
        }
    }

    virtual void displayPrint(String content, bool ln = false, bool flush = true) {
        if (ln) {
            max.println(content);
        } else {
            max.print(content);
        }
        if (flush) {
            max.write();
        }
    }
    virtual void displayFormat(int16_t x, int16_t y, int16_t w, int16_t align, String content,
                               bool flush = true) {
        max.printFormatted(x, y, w, align, content);
        if (flush) {
            max.write();
        }
    }
};

const char *DisplayDigitsMAX72XX::version = "0.1.0";

}  // namespace ustd