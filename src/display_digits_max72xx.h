// display_digits_max72xx.h - mupplet for 7 segment digits display using MAX72xx

#pragma once

#include "ustd_array.h"
#include "muwerk.h"
#include "helper/light_controller.h"
#include "hardware/max72xx_digits.h"

namespace ustd {

class DisplayDigitsMAX72XX {
  public:
    static const char *version;  // = "0.1.0";
    static const char *formatTokens[];

  private:
    // muwerk task management
    Scheduler *pSched;
    int tID;

    // device configuration
    String name;

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
        : name(name), max(csPin, hDisplays, vDisplays, length) {
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

    bool commandParser(String command, String args, String topic) {
        if (command.startsWith("cmnd/")) {
            return commandCmdParser(command.substring(5), args);
        } else if (command.startsWith("cursor/")) {
            return cursorParser(command.substring(7), args, topic + "/cursor");
        } else if (command.startsWith("wrap/")) {
            return wrapParser(command.substring(5), args, topic + "/wrap");
        }
        return false;
    }

    bool cursorParser(String command, String args, String topic) {
        if (command == "get") {
            pSched->publish(topic, String(max.getCursorX()) + ";" + String(max.getCursorY()));
            return true;
        } else if (command == "set") {
            max.setCursor(
                parseRangedLong(shift(args, ';'), 0, max.width() - 1, 0, max.width() - 1),
                parseRangedLong(shift(args, ';'), 0, max.height() - 1, 0, max.height() - 1));
            pSched->publish(topic, String(max.getCursorX()) + ";" + String(max.getCursorY()));
            return true;
        } else if (command == "x/get") {
            pSched->publish(topic + "/x", String(max.getCursorX()));
            return true;
        } else if (command == "x/set") {
            max.setCursorX(
                parseRangedLong(shift(args, ';'), 0, max.width() - 1, 0, max.width() - 1));
            pSched->publish(topic + "/x", String(max.getCursorX()));
            return true;
        } else if (command == "y/get") {
            pSched->publish(topic + "/x", String(max.getCursorY()));
            return true;
        } else if (command == "y/set") {
            max.setCursorX(
                parseRangedLong(shift(args, ';'), 0, max.height() - 1, 0, max.height() - 1));
            pSched->publish(topic + "/x", String(max.getCursorY()));
            return true;
        }
        return false;
    }

    bool wrapParser(String command, String args, String topic) {
        if (command == "get") {
            pSched->publish(topic, max.getTextWrap() ? "on" : "off");
            return true;
        } else if (command == "set") {
            int8_t wrap = parseBoolean(args);
            if (wrap >= 0) {
                max.setTextWrap(wrap == 1);
                pSched->publish(topic, max.getTextWrap() ? "on" : "off");
                return true;
            }
        }
        return false;
    }

    bool commandCmdParser(String command, String args) {
        int16_t x, y, w, h;
        array<String> params;
        if (command == "clear") {
            if (args.length()) {
                split(args, ';', params);
            }
            switch (params.length()) {
            case 0:
                // clear the whole screen
                displayClear();
                return true;
            case 1:
                // clear the specified line
                y = parseLong(params[0], 0);
                max.fillRect(0, y, max.width(), 1, 0);
                max.write();
                return true;
            case 2:
                // clear the rect from specified coordinates
                x = parseLong(params[0], 0);
                y = parseLong(params[1], 0);
                max.fillRect(x, y, max.width(), max.height(), 0);
                max.write();
                return true;
            case 4:
                // clear the rect with specified coordinates and size
                x = parseLong(params[0], 0);
                y = parseLong(params[1], 0);
                w = parseLong(params[2], 0);
                h = parseLong(params[3], 0);
                max.fillRect(x, y, w, h, 0);
                max.write();
                return true;
            }
        } else if (command == "print") {
            max.print(args);
            max.write();
        } else if (command == "println") {
            max.println(args);
            max.write();
        } else if (command == "printat") {
            x = parseRangedLong(shift(args, ';', "0"), 0, max.width() - 1, 0, max.width() - 1);
            y = parseRangedLong(shift(args, ';', "0"), 0, max.height() - 1, 0, max.height() - 1);
            max.setCursor(x, y);
            max.print(args);
            max.write();
        } else if (command == "format") {
            x = parseLong(shift(args, ';', ""), 0);
            y = parseLong(shift(args, ';', ""), 0);
            w = parseLong(shift(args, ';', ""), 0);
            h = parseToken(shift(args, ';', "left"), formatTokens);
            max.printFormatted(x, y, w, h, args);
            max.write();
        }
        return false;
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

    void displayClear(bool flush = true) {
        max.fillScreen(0);
        max.setCursor(0, 0);
        if (flush) {
            max.write();
        }
    }

    int16_t getTextWidth(const char *str) {
        uint8_t c;  // Current character
        int16_t len = 0;
        while ((c = *str++)) {
            len += max.getCharLen(c);
        }
        return len;
    }
};

const char *DisplayDigitsMAX72XX::version = "0.1.0";
const char *DisplayDigitsMAX72XX::formatTokens[] = {"left", "center", "right", nullptr};

}  // namespace ustd