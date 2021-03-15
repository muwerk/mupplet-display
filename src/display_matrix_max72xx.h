// display_matrix_max72xx.h - mupplet for Matrix Display using MAX72xx

#pragma once

#include "ustd_array.h"
#include "timeout.h"
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
        : MuppletGfxDisplay(name), display(csPin, hDisplays, vDisplays, rotation) {
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

    virtual void displayClear(int16_t x, int16_t y, int16_t w, int16_t h, bool flush = true) {
        display.fillRect(x, y, w, h, 0);
        if (flush) {
            display.write();
        }
    }

    virtual void displayPrint(String content, bool ln = false, bool flush = true) {
        if (ln) {
            display.println(content);
        } else {
            display.print(content);
        }
        if (flush) {
            display.write();
        }
    }

    virtual bool displayFormat(int16_t x, int16_t y, int16_t w, int16_t align, String content,
                               bool flush = true) {
        bool ret = display.printFormatted(x, y, w, align, content, sizes[current_font].baseLine);
        if (flush) {
            display.write();
        }
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

#ifdef KAKKA
class DisplayMatrixMAX72XX {
  public:
    static const char *version;  // = "0.1.0";
    static const char *modeTokens[];

    /// Program Item Display Mode
    enum Mode {
        Left,     ///< Static left formatted text
        Center,   ///< Static centered text
        Right,    ///< Static right formatted text
        SlideIn,  ///< Text slides in char per char to the left side
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
    Max72xxMatrix display;

    // runtime
    LightController light;
    static const GFXfont *default_font;
    ustd::array<const GFXfont *> fonts;
    ustd::array<FontSize> sizes;
    // runtime - program control
    ustd::array<ProgramItem> program;
    ProgramItem default_item;
    int16_t program_counter;
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
    /*! Instantiates a DisplayMatrixMAX72XX mupplet
     *
     * No hardware interaction is performed, until \ref begin() is called.
     *
     * @param name Name of the led, used to reference it by pub/sub messages
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
        : name(name), display(csPin, hDisplays, vDisplays, rotation), fonts(4, ARRAY_MAX_SIZE, 4) {
        FontSize default_size = {0, 6, 8, 0};
        fonts.add(default_font);
        sizes.add(default_size);
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
        display.begin();
        display.setPowerSave(false);
        display.setIntensity(8);

        // start light controller
        light.begin([this](bool state, double level, bool control,
                           bool notify) { this->onLightControl(state, level, control, notify); },
                    initialState);
    }

    /*! Adds an Adafruit GFX font to the display mupplet
     * @param font      The Adafruit GFXfont object
     * @param baseLine  The baseline value of the selected font
     */
    void addfont(const GFXfont *font, uint8_t baseLine) {
        FontSize size = {baseLine, 0, 0, 0};
        getFontSize(font, size);
        fonts.add(font);
        sizes.add(size);
    }

    /*! Adds an Adafruit GFX font to the display mupplet
     * @param font              The Adafruit GFXfont object
     * @param baseLineReference The reference char used to determine the baseline value of the
     *                          selected font
     */
    void addfont(const GFXfont *font, const char *baseLineReference = "A") {
        FontSize size = {0, 0, 0, 0};
        getFontSize(font, size, *baseLineReference);
        fonts.add(font);
        sizes.add(size);
    }

    /*! Set the internal default values for program items
     * @param mode      Display mode of the program item
     * @param dur       Duration in milliseconds of the program item
     * @param repeat    Number of repetitions for the program item. Set `0` for infinite repetitions
     * @param speed     Effect speed of the program item. Value range: 1 (slow) to 16 (fast)
     * @param font      Font index of program item. Set `0` for builtin font.
     */
    void setDefaults(Mode mode, unsigned long dur, int16_t repeat, uint8_t speed, uint8_t font) {
        default_item.mode = mode;
        default_item.duration = dur;
        default_item.repeat = repeat;
        default_item.font = font > fonts.length() ? 0 : font;
        default_item.speed = speed > 16 ? 16 : speed;
    }

    /*! Remove all program items
     */
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
                lastPos = display.getCursorX();
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
            slidePos = display.width();
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
            display.fillRect(slidePos, 0, slidePos + charX, charY, 0);
            // draw char at new position
            // slidePos = (slidePos - speed) < lastPos ? lastPos : slidePos - speed;
            slidePos--;
            display.drawChar(slidePos, sizes[item.font].baseLine, item.content[charPos], 1, 0, 1);
            DBG3("Writing " + String(content[charPos]) + " at position " + String(slidePos));
            // flush to display
            display.write();
            // prepare for next iterations:
            if (slidePos <= lastPos) {
                // char has arrived
                lastPos += charX;
                slidePos = display.width();
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
        if (program_counter >= (int16_t)program.length()) {
            program_counter = 0;
        }
    }

    void displaySetFont(ProgramItem &item) {
        display.setFont(fonts[item.font]);
        display.setCursor(display.getCursorX(), sizes[item.font].baseLine);
    }

    void displayClear(ProgramItem &item, bool flush = true) {
        display.fillScreen(0);
        display.setFont(fonts[item.font]);
        display.setCursor(0, sizes[item.font].baseLine);
        if (flush) {
            display.write();
        }
    }

    void displayLeft(ProgramItem &item) {
        displayClear(item, false);
        display.print(item.content);
        display.write();
    }

    void displayCenter(ProgramItem &item) {
        displayClear(item, false);
        int16_t width = getTextWidth(item);
        display.setCursor((display.width() - width) / 2, display.getCursorY());
        display.print(item.content);
        display.write();
    }

    void displayRight(ProgramItem &item) {
        displayClear(item, false);
        int16_t width = getTextWidth(item);
        display.setCursor(-width, display.getCursorY());
        display.print(item.content);
        display.write();
    }

    int16_t getTextWidth(ProgramItem &item) {
        int16_t x, y;
        uint16_t w, h;
        display.getTextBounds(item.content, 0, display.getCursorY(), &x, &y, &w, &h);
        if ((int16_t)w < display.width()) {
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
            // } else if (command == "testmode") {
            //     display.setTestMode(args == "on");
            // } else if (command == "intensity") {
            //     display.setIntensity(parseRangedLong(args, 0, 15, 0, 15));
            // } else if (command == "zerofill") {
            //     display.fillScreen(0);
            //     display.write();
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
            } else if (operation == "jump") {
                if (jumpItem(index)) {
                    publishItem(index);
                }
            } else if (operation == "clear") {
                if (clearItem(index)) {
                    publishItemsCount();
                }
            }
        }
    }

    void commandContentParser(String command, String args) {
        String name, operation;
        if (command == "clear") {
            clearItems();
            publishItemsCount();
        } else if (command == "get") {
            publishContents();
        } else if (command == "add") {
            addContent("unnamed_" + String(++anonymous_counter), args);
            publishItemsCount();
        } else if (parseItemCommand(command, name, operation)) {
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
            } else if (operation == "jump") {
                if (jumpItem(index)) {
                    publishContent(index);
                }
            } else if (operation == "clear") {
                if (clearItem(index)) {
                    publishItemsCount();
                }
            }
        }
    }

    void commandValueParser(String command, String args) {
    }

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

    bool jumpItem(int16_t i) {
        if (i < 0 || i > (int16_t)program.length() - 1) {
            return false;
        }
        program_counter = i;
        program_state = None;
        return true;
    }

    bool clearItem(int16_t i) {
        if (i < 0 || i > (int16_t)program.length() - 1) {
            return false;
        }
        program.erase(i);
        if (program_counter > i) {
            // adjust program counter
            program_counter--;
        } else if (program_counter == i) {
            // the deleted item was the current item. program counter is OK, but we need to reset
            // the sequence in order to start the next item immediately
            program_state = None;
            if (program_counter >= (int16_t)program.length()) {
                program_counter = 0;
            }
        }
        if (program.length() == 0) {
            displayClear(default_item);
        }
        return true;
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

    void publishContents() {
        for (unsigned int i = 0; i < program.length(); i++) {
            pSched->publish(name + "/display/content/" + program[i].name, program[i].content);
        }
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
            display.setIntensity(intensity);
        }
        if (control) {
            display.setPowerSave(!state);
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
#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
        return &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c]);
#else
        return &(((GFXglyph *)pgm_read_word(&gfxFont->glyph))[c]);
#endif
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
#endif

}  // namespace ustd