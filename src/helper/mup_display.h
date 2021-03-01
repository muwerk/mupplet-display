// mup_display.h - mupplet display base class

#pragma once

#include "muwerk.h"
#include "scheduler.h"

namespace ustd {
class MuppletDisplay {
  public:
    static const char *formatTokens[];

  protected:
    // font helper
    typedef struct {
        uint8_t baseLine;
        uint8_t xAdvance;
        uint8_t yAdvance;
        uint8_t dummy;
    } FontSize;

    // muwerk task management
    Scheduler *pSched;
    int tID;

    // device configuration
    String name;

  public:
    MuppletDisplay(String name) : name(name) {
    }

  protected:
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

    bool commandCmdParser(String command, String args) {
        int16_t x, y, w, h, width, height;
        array<String> params;
        if (command == "clear") {
            if (args.length()) {
                split(args, ';', params);
            }
            getDimensions(width, height);
            FontSize fs = getTextFontSize();
            switch (params.length()) {
            case 0:
                // clear the whole screen
                displayClear(0, 0, width, height);
                setCursor(0, 0 + fs.baseLine);
                return true;
            case 1:
                // clear the specified line
                y = parseLong(params[0], 0);
                displayClear(0, y, width, fs.yAdvance);
                setCursor(0, y + fs.baseLine);
                return true;
            case 2:
                // clear the rect from specified coordinates
                x = parseLong(params[0], 0);
                y = parseLong(params[1], 0);
                displayClear(x, y, width, height);
                setCursor(x, y + fs.baseLine);
                return true;
            case 4:
                // clear the rect with specified coordinates and size
                x = parseLong(params[0], 0);
                y = parseLong(params[1], 0);
                w = parseLong(params[2], 0);
                h = parseLong(params[3], 0);
                displayClear(x, y, w, h);
                setCursor(x, y + fs.baseLine);
                return true;
            }
        } else if (command == "print") {
            displayPrint(args);
            return true;
        } else if (command == "println") {
            displayPrint(args, true);
            return true;
        } else if (command == "printat") {
            getDimensions(width, height);
            x = parseRangedLong(shift(args, ';', "0"), 0, width - 1, 0, width - 1);
            y = parseRangedLong(shift(args, ';', "0"), 0, height - 1, 0, height - 1);
            setCursor(x, y);
            displayPrint(args);
            return true;
        } else if (command == "format") {
            getDimensions(width, height);
            x = parseLong(shift(args, ';', ""), 0);
            y = parseLong(shift(args, ';', ""), 0);
            w = parseLong(shift(args, ';', ""), width);
            h = parseToken(shift(args, ';', "left"), formatTokens);
            displayFormat(x, y, w, h, args);
            return true;
        }
        return false;
    }

    bool cursorParser(String command, String args, String topic) {
        int16_t x, y, width, height;
        if (command == "get") {
            getCursor(x, y);
            pSched->publish(topic, String(x) + ";" + String(y));
            return true;
        } else if (command == "set") {
            getDimensions(width, height);
            setCursor(parseRangedLong(shift(args, ';'), 0, width - 1, 0, width - 1),
                      parseRangedLong(shift(args, ';'), 0, height - 1, 0, height - 1));
            getCursor(x, y);
            pSched->publish(topic, String(x) + ";" + String(y));
            return true;
        } else if (command == "x/get") {
            getCursor(x, y);
            pSched->publish(topic + "/x", String(x));
            return true;
        } else if (command == "x/set") {
            getCursor(x, y);
            getDimensions(width, height);
            setCursor(parseRangedLong(shift(args, ';'), 0, width - 1, 0, width - 1), y);
            getCursor(x, y);
            pSched->publish(topic + "/x", String(x));
            return true;
        } else if (command == "y/get") {
            getCursor(x, y);
            pSched->publish(topic + "/y", String(y));
        } else if (command == "y/set") {
            getCursor(x, y);
            getDimensions(width, height);
            setCursor(x, parseRangedLong(shift(args, ';'), 0, height - 1, 0, height - 1));
            getCursor(x, y);
            pSched->publish(topic + "/y", String(y));
            return true;
        }
        return false;
    }

    bool wrapParser(String command, String args, String topic) {
        if (command == "get") {
            pSched->publish(topic, getTextWrap() ? "on" : "off");
            return true;
        } else if (command == "set") {
            int8_t wrap = parseBoolean(args);
            if (wrap >= 0) {
                setTextWrap(wrap == 1);
                pSched->publish(topic, getTextWrap() ? "on" : "off");
                return true;
            }
        }
        return false;
    }

    virtual void getDimensions(int16_t &width, int16_t &height) = 0;
    virtual bool getTextWrap() = 0;
    virtual void setTextWrap(bool wrap) = 0;
    virtual uint8_t getTextFont() = 0;
    virtual FontSize getTextFontSize() = 0;
    virtual void setTextFont(uint8_t font) = 0;
    virtual void getCursor(int16_t &x, int16_t &y) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    virtual void displayClear(int16_t x, int16_t y, int16_t w, int16_t h, bool flush = true) = 0;
    virtual void displayPrint(String content, bool ln = false, bool flush = true) = 0;
    virtual void displayFormat(int16_t x, int16_t y, int16_t w, int16_t align, String content,
                               bool flush = true) = 0;
};

const char *MuppletDisplay::formatTokens[] = {"left", "center", "right", nullptr};

}  // namespace ustd