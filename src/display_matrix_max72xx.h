// display_matrix_max72xx.h - mupplet for Matrix Display using MAX72xx

#pragma once

#include "ustd_array.h"
#include "timeout.h"
#include "muwerk.h"
#include "helper/light_controller.h"
#include "hardware/max72xx_matrix.h"

namespace ustd {

class DisplayMatrixMAX72XX {
  public:
    static const char *version;  // = "0.1.0";
    static const char *modeTokens[];
    enum Mode {
        Left,
        Center,
        Right,
        SlideIn,
    };

  private:
    // font helper
    typedef struct {
        uint8_t baseLine;
        uint8_t xAdvance;
        uint8_t yAdvance;
        uint8_t dummy;
    } FontSize;

    // program item
    typedef struct {
        String name;
        Mode mode;
        ustd::timeout duration;
        int16_t repeat;
        uint8_t speed;
        uint8_t font;
        String content;
    } ProgramItem;

    // state
    enum State { None, FadeIn, Wait, FadeOut, Finished };

    // muwerk task management
    Scheduler *pSched;
    int tID;

    // device configuration
    String name;

    // hardware configuration
    max72xx max;

    // runtime
    LightController light;
    static const GFXfont *default_font;
    ustd::array<const GFXfont *> fonts;
    ustd::array<FontSize> sizes;
    // runtime - program control
    ustd::array<ProgramItem> program;
    ProgramItem default_item;
    uint16_t program_counter;
    State program_state;
    unsigned long anonymous_counter;
    // runtime - effect control
    uint8_t delayCtr;  // effect delay counter
    uint16_t charPos;  // index of char to slide
    uint8_t lastPos;   // target position of sliding char
    uint8_t slidePos;  // position of sliding char
    uint8_t charX;     // width of current char
    uint8_t charY;     // height of current char

  public:
    DisplayMatrixMAX72XX(String name, uint8_t csPin = D8, uint8_t hDisplays = 1,
                         uint8_t vDisplays = 1, uint8_t rotation = 0)
        : name(name), max(csPin, hDisplays, vDisplays, rotation), fonts(4, ARRAY_MAX_SIZE, 4) {
        FontSize default_size = {0, 6, 8, 0};
        fonts.add(default_font);
        sizes.add(default_size);
    }

    void begin(Scheduler *_pSched, bool initialState = false) {
        pSched = _pSched;
        tID = pSched->add([this]() { this->loop(); }, name, 10000L);

        pSched->subscribe(tID, name + "/display/#", [this](String topic, String msg, String orig) {
            this->commandParser(topic.substring(name.length() + 9), msg);
        });
        pSched->subscribe(tID, name + "/light/#", [this](String topic, String msg, String orig) {
            this->light.commandParser(topic.substring(name.length() + 7), msg);
        });

        // initialize default values
        default_item.mode = Left;
        default_item.duration = 2000;
        default_item.speed = 16;
        default_item.font = 0;
        default_item.repeat = 1;

        // initialize state machine
        program_counter = 0;
        program_state = None;
        anonymous_counter = 0;

        // prepare hardware
        max.begin();
        max.fillScreen(0);
        max.write();
        max.setPowerSave(false);
        max.setIntensity(8);

        // start light controller
        light.begin([this](bool state, double level, bool control,
                           bool notify) { this->onLightControl(state, level, control, notify); },
                    initialState);
    }

    void addfont(const GFXfont *font, uint8_t baseLine) {
        FontSize size = {baseLine, 0, 0, 0};
        getFontSize(font, size);
        fonts.add(font);
        sizes.add(size);
    }

    void addfont(const GFXfont *font, const char *baseLineReference = "A") {
        FontSize size = {0, 0, 0, 0};
        getFontSize(font, size, *baseLineReference);
        fonts.add(font);
        sizes.add(size);
    }

    void setDefaults(Mode mode, unsigned long dur, int16_t repeat, uint8_t speed, uint8_t font) {
        default_item.mode = mode;
        default_item.duration = dur;
        default_item.repeat = repeat;
        default_item.font = font > fonts.length() ? 0 : font;
        default_item.speed = speed > 16 ? 16 : speed;
    }

    void clearItems() {
        program.erase();
        program_counter = 0;
        program_state = None;
        displayClear(default_item);
    }

  private:
    void loop() {
        light.loop();
        if (program.length() == 0) {
            return;
        }
        if (program_state == None) {
            startProgramItem(program[program_counter]);
        }
        if (program_state == FadeIn) {
            fadeInProgramItem(program[program_counter]);
        }
        if (program_state == Wait) {
            waitProgramItem(program[program_counter]);
        }
        if (program_state == FadeOut) {
            fadeOutProgramItem(program[program_counter]);
        }
        if (program_state == Finished) {
            endProgramItem(program[program_counter]);
        }
        if (program.length() == 0) {
            displayClear(default_item);
        }
    }

    void changedProgramItem(ProgramItem &item) {
        switch (item.mode) {
        case Left:
            displayLeft(item);
            break;
        case Center:
            displayCenter(item);
            break;
        case Right:
            displayRight(item);
            break;
        case SlideIn:
            if (program_state == FadeIn && charPos < item.content.length() - 1) {
                String temp = item.content;
                item.content = item.content.substring(0, charPos);
                displayLeft(item);
                item.content = temp;
                lastPos = max.getCursorX();
                initNextCharDimensions(item);
            } else {
                displayLeft(item);
                program_state = Wait;
            }
            break;
        default:
            break;
        }
    }

    void startProgramItem(ProgramItem &item) {
        item.duration.reset();
        switch (item.mode) {
        case Left:
            displayLeft(item);
            program_state = Wait;
            break;
        case Center:
            displayCenter(item);
            program_state = Wait;
            break;
        case Right:
            displayRight(item);
            program_state = Wait;
            break;
        case SlideIn:
            charPos = 0;
            lastPos = 0;
            delayCtr = 17 - item.speed;
            slidePos = max.width();
            if (initNextCharDimensions(item)) {
                displayClear(item);
                program_state = FadeIn;
            } else {
                displayLeft(item);
                program_state = Wait;
            }
            break;
        default:
            program_state = Finished;
            break;
        }
    }

    void fadeInProgramItem(ProgramItem &item) {
        if (item.mode == SlideIn) {
            if (--delayCtr) {
                return;
            }
            delayCtr = 17 - item.speed;
            // clear char at previous position
            max.fillRect(slidePos, 0, slidePos + charX, charY, 0);
            // draw char at new position
            // slidePos = (slidePos - speed) < lastPos ? lastPos : slidePos - speed;
            slidePos--;
            max.drawChar(slidePos, sizes[item.font].baseLine, item.content[charPos], 1, 0, 1);
            DBG3("Writing " + String(content[charPos]) + " at position " + String(slidePos));
            // flush to display
            max.write();
            // prepare for next iterations:
            if (slidePos <= lastPos) {
                // char has arrived
                lastPos += charX;
                slidePos = max.width();
                if (lastPos >= slidePos) {
                    // display full
                    fadeInEnd(item);
                    return;
                }
                ++charPos;
                if (!initNextCharDimensions(item)) {
                    // end of string
                    fadeInEnd(item);
                    return;
                }
            }
        } else {
            fadeInEnd(item);
            return;
        }
    }

    void fadeInEnd(ProgramItem &item) {
        item.duration.reset();
        program_state = Wait;
    }

    void waitProgramItem(ProgramItem &item) {
        if (item.duration.test()) {
            program_state = FadeOut;
        }
    }

    void fadeOutProgramItem(ProgramItem &item) {
        // switch (item.mode) {
        // case Left:
        // case Center:
        // case Right:
        // case SlideIn:
        // default:
        // }
        fadeOutEnd(item);
    }

    void fadeOutEnd(ProgramItem &item) {
        program_state = Finished;
    }

    void endProgramItem(ProgramItem &item) {
        program_state = None;
        if (item.repeat) {
            if (--item.repeat) {
                // item still active -> skip to next
                program_counter++;
            } else {
                // remove current item from program
                program.erase(program_counter);
            }
        } else {
            program_counter++;
        }
        if (program_counter >= program.length()) {
            program_counter = 0;
        }
    }

    void displaySetFont(ProgramItem &item) {
        max.setFont(fonts[item.font]);
        max.setCursor(max.getCursorX(), sizes[item.font].baseLine);
    }

    void displayClear(ProgramItem &item, bool flush = true) {
        max.fillScreen(0);
        max.setFont(fonts[item.font]);
        max.setCursor(0, sizes[item.font].baseLine);
        if (flush) {
            max.write();
        }
    }

    void displayLeft(ProgramItem &item) {
        displayClear(item, false);
        max.print(item.content);
        max.write();
    }

    void displayCenter(ProgramItem &item) {
        displayClear(item, false);
        int16_t width = getTextWidth(item);
        max.setCursor((max.width() - width) / 2, max.getCursorY());
        max.print(item.content);
        max.write();
    }

    void displayRight(ProgramItem &item) {
        displayClear(item, false);
        int16_t width = getTextWidth(item);
        max.setCursor(-width, max.getCursorY());
        max.print(item.content);
        max.write();
    }

    int16_t getTextWidth(ProgramItem &item) {
        int16_t x, y;
        uint16_t w, h;
        max.getTextBounds(item.content, 0, max.getCursorY(), &x, &y, &w, &h);
        if (w < max.width()) {
            return w;
        } else {
            GFXcanvas1 tmp(item.content.length() * sizes[item.font].xAdvance,
                           sizes[item.font].yAdvance);
            if (item.font != 0) {
                tmp.setFont(fonts[item.font]);
            }
            tmp.print(item.content);
            return tmp.getCursorX();
        }
    }

    bool initNextCharDimensions(ProgramItem &item) {
        while (charPos < item.content.length()) {
            int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1;
            int16_t x = 0, y = sizes[item.font].baseLine;
            max.getCharBounds(item.content[charPos], &x, &y, &minx, &miny, &maxx, &maxy);
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

    void commandParser(String command, String args) {
        if (command == "count/get") {
            publishItemsCount();
        } else if (command.startsWith("default/")) {
            commandDefaultParser(command.substring(8), args);
        } else if (command.startsWith("items/")) {
            commandItemsParser(command.substring(6), args);
        } else if (command.startsWith("content/")) {
            commandContentParser(command.substring(8), args);
        } else if (command == "on") {
            light.set(true);
        } else if (command == "off") {
            light.set(false);
        }
    }

    void commandDefaultParser(String command, String args) {
        if (command == "mode/set") {
            if (parseMode(args, default_item)) {
                publishDefaultMode();
            }
        } else if (command == "mode/get") {
            publishDefaultMode();
        } else if (command == "repeat/set") {
            if (parseRepeat(args, default_item)) {
                publishDefaultRepeat();
            }
        } else if (command == "repeat/get") {
            publishDefaultRepeat();
        } else if (command == "duration/set") {
            if (parseDuration(args, default_item)) {
                publishDefaultDuration();
            }
        } else if (command == "duration/get") {
            publishDefaultDuration();
        } else if (command == "speed/set") {
            if (parseSpeed(args, default_item)) {
                publishDefaultSpeed();
            }
        } else if (command == "speed/get") {
            publishDefaultSpeed();
        } else if (command == "font/set") {
            if (parseFont(args, default_item)) {
                publishDefaultFont();
            }
        } else if (command == "font/get") {
            publishDefaultFont();
        } else if (command == "set") {
            if (parseDefaults(args)) {
                publishDefaults();
            }
        } else if (command == "get") {
            publishDefaults();
        }
    }

    void commandItemsParser(String command, String args) {
        String name, operation;
        if (command == "clear") {
            clearItems();
            publishItemsCount();
        } else if (command == "get") {
            publishItems();
        } else if (command == "add") {
            addItem("unnamed_" + String(++anonymous_counter), args);
            publishItemsCount();
        } else if (parseItemCommand(command, name, operation)) {
            int16_t index = findItemByName(name.c_str());
            if (operation == "set") {
                if (index < 0) {
                    index = addItem(name, args);
                } else {
                    index = replaceItem(index, args);
                }
                if (index >= 0) {
                    publishItem(index);
                }
            } else if (operation == "get") {
                publishItem(index);
            } else if (operation == "clear") {
                clearItem(index);
            }
        }
    }

    void commandContentParser(String command, String args) {
        String name, operation;
        if (parseItemCommand(command, name, operation)) {
            int16_t index = findItemByName(name.c_str());
            if (operation == "set") {
                if (index < 0) {
                    index = addContent(name, args);
                } else {
                    index = replaceContent(index, args);
                }
                if (index >= 0) {
                    publishContent(index);
                }
            } else if (operation == "get") {
                publishContent(index);
            } else if (operation == "clear") {
                clearItem(index);
            }
        }
    }

    void commandValueParser(String command, String args) {
    }
    /*
            if (command == "KAKAK") {
            } else             } else if (command == "content/add") {
                addContent("unnamed_" + String(++anonymous_counter), args);
                pSched->publish(name + "/display/items/count", String(program.length()));
            } else if (command == "on") {
                light.set(true);
            } else if (command == "off") {
                light.set(false);
            } else {
                String name, operation;
                if (parseItemCommand(command, name, operation)) {
                    if (operation == "set") {
                    } else if (operation == "get") {
                    } else if (operation == "clear") {
                    } else if (operation == "clear") {
                    }
                }
            }
        }
    */
    bool parseItemCommand(String command, String &name, String &operation) {
        const char *pPtr = command.c_str();
        const char *pOperation = strrchr(pPtr, '/');
        if (!pOperation) {
            return false;
        }
        size_t len = pOperation - pPtr;
        operation = pOperation + 1;
        name = pPtr;
        name.remove(len);
        return name.length() && operation.length();
    }

    bool parseDefaults(String args) {
        bool changed = parseMode(shift(args, ';'), default_item);
        changed = changed || parseRepeat(shift(args, ';'), default_item);
        changed = changed || parseDuration(shift(args, ';'), default_item);
        changed = changed || parseSpeed(shift(args, ';'), default_item);
        changed = changed || parseFont(shift(args, ';'), default_item);
        return changed;
    }

    void publishDefaults() {
        pSched->publish(name + "/display/default", getItemString(default_item));
    }

    bool parseMode(String args, ProgramItem &item) {
        int16_t iMode = parseToken(args, modeTokens);
        if (iMode >= 0 && iMode != item.mode) {
            item.mode = (Mode)iMode;
            return true;
        }
        return false;
    }

    void publishDefaultMode() {
        pSched->publish(name + "/display/default/mode", modeTokens[default_item.mode]);
    }

    bool parseRepeat(String args, ProgramItem &item) {
        if (args.length()) {
            int16_t value = parseRangedLong(args, 0, 32767, 0, 32767);
            if (value != item.repeat) {
                item.repeat = value;
                return true;
            }
        }
        return false;
    }

    void publishDefaultRepeat() {
        pSched->publish(name + "/display/default/repeat", String(default_item.repeat));
    }

    bool parseDuration(String args, ProgramItem &item) {
        if (args.length()) {
            long value = atol(args.c_str());
            if (value >= 0 && (unsigned long)value != item.duration) {
                item.duration = value;
                return true;
            }
        }
        return false;
    }

    void publishDefaultDuration() {
        pSched->publish(name + "/display/default/duration", String(default_item.duration));
    }

    bool parseSpeed(String args, ProgramItem &item) {
        if (args.length()) {
            long value = parseRangedLong(args, 1, 16, 0, 16);
            if (value && value != item.speed) {
                item.speed = value;
                return true;
            }
        }
        return false;
    }

    void publishDefaultSpeed() {
        pSched->publish(name + "/display/default/speed", String(default_item.speed));
    }

    bool parseFont(String args, ProgramItem &item) {
        if (args.length()) {
            int value = atoi(args.c_str());
            if (value >= 0 && value < (int)fonts.length() && value != item.font) {
                item.font = value;
                return true;
            }
        }
        return false;
    }

    void publishDefaultFont() {
        pSched->publish(name + "/display/default/font", String(default_item.font));
    }

    void publishItemsCount() {
        pSched->publish(name + "/display/count", String(program.length()));
    }

    int16_t addItem(String name, String args) {
        ProgramItem item = default_item;
        item.name = name;
        parseMode(shift(args, ';'), item);
        parseRepeat(shift(args, ';'), item);
        parseDuration(shift(args, ';'), item);
        parseSpeed(shift(args, ';'), item);
        parseFont(shift(args, ';'), item);
        item.content = args;
        return program.add(item);
    }

    int16_t replaceItem(int16_t i, String args) {
        if (i < 0 || i > (int16_t)program.length() - 1) {
            return -1;
        }
        ProgramItem item = program[i];
        parseMode(shift(args, ';'), item);
        parseRepeat(shift(args, ';'), item);
        parseDuration(shift(args, ';'), item);
        parseSpeed(shift(args, ';'), item);
        parseFont(shift(args, ';'), item);
        item.content = args;
        program[i] = item;
        if (program_counter == i) {
            changedProgramItem(program[i]);
        }
        return i;
    }

    void clearItem(int16_t i) {
        if (i < 0 || i > (int16_t)program.length() - 1) {
            return;
        }
        program.erase(i);
        if (program_counter > i) {
            // adjust program counter
            program_counter--;
        } else if (program_counter == i) {
            // the deleted item was the current item. program counter is OK, but we need to reset
            // the sequence in order to start the next item immediately
            program_state = None;
            if (program_counter >= program.length()) {
                program_counter = 0;
            }
        }
    }

    void publishItem(int16_t i) {
        if (i < 0 || i > (int16_t)program.length() - 1) {
            return;
        }
        pSched->publish(name + "/display/items/" + program[i].name, getItemString(program[i]));
    }

    void publishItems() {
        for (unsigned int i = 0; i < program.length(); i++) {
            pSched->publish(name + "/display/items/" + program[i].name, getItemString(program[i]));
        }
    }

    int16_t addContent(String name, String args) {
        ProgramItem item = default_item;
        item.name = name;
        item.content = args;
        program.add(item);
        return program.length();
    }

    int16_t replaceContent(int16_t i, String args) {
        if (i < 0 || i > (int16_t)program.length() - 1) {
            return -1;
        }
        program[i].content = args;
        if (program_counter == i) {
            changedProgramItem(program[i]);
        }
        return i;
    }

    void publishContent(int16_t i) {
        if (i < 0 || i > (int16_t)program.length() - 1) {
            return;
        }
        pSched->publish(name + "/display/content/" + program[i].name, program[i].content);
    }

    int16_t findItemByName(const char *name) {
        for (int16_t i = 0; i < (int16_t)program.length(); i++) {
            if (program[i].name == name) {
                return i;
            }
        }
        return -1;
    }

    String getItemString(ProgramItem &item) {
        String itemString = modeTokens[item.mode];
        itemString.concat(";");
        itemString.concat(item.repeat);
        itemString.concat(";");
        itemString.concat(item.duration);
        itemString.concat(";");
        itemString.concat(item.speed);
        itemString.concat(";");
        itemString.concat(item.font);
        if (item.content.length()) {
            itemString.concat(";");
            itemString.concat(item.content);
        }
        return itemString;
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
            //     char buf[32];
            //     sprintf(buf, "%5.3f", level);
            //     pSched->publish(name + "/light/unitbrightness", buf);
            pSched->publish(name + "/light/unitbrightness", String(level, 3));
            pSched->publish(name + "/light/state", state ? "on" : "off");
        }
    }

    void getFontSize(const GFXfont *font, FontSize &size, char baseLineChar = 0) {
        uint8_t first = pgm_read_byte(&font->first);
        uint8_t last = pgm_read_byte(&font->last);
        uint16_t baselineGlyphIndex = (uint16_t)baseLineChar > first ? baseLineChar - first : 0;
        uint16_t glyphs = last - first + 1;

        for (uint16_t i = 0; i < glyphs; i++) {
            GFXglyph *glyph = pgm_read_glyph_ptr(font, i);
            uint8_t xAdvance = (uint8_t)pgm_read_byte(&glyph->xAdvance);

            if (baseLineChar && i == baselineGlyphIndex) {
                size.baseLine = (int8_t)pgm_read_byte(&glyph->yOffset) * -1;
            }
            if (xAdvance > size.xAdvance) {
                size.xAdvance = xAdvance;
            }
        }
        size.yAdvance = (uint8_t)pgm_read_byte(&font->yAdvance);
    }

    static GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
#ifdef __AVR__
        return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
#else
        // expression in __AVR__ section may generate "dereferencing type-punned
        // pointer will break strict-aliasing rules" warning In fact, on other
        // platforms (such as STM32) there is no need to do this pointer magic as
        // program memory may be read in a usual way So expression may be simplified
        return gfxFont->glyph + c;
#endif  //__AVR__
    }
};

const char *DisplayMatrixMAX72XX::version = "0.1.0";
const char *DisplayMatrixMAX72XX::modeTokens[] = {"left", "center", "right", "slidein", nullptr};
const GFXfont *DisplayMatrixMAX72XX::default_font = nullptr;

}  // namespace ustd