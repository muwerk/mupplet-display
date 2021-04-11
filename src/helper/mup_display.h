// mup_display.h - mupplet display base class

#pragma once

#ifdef USTD_FEATURE_PROGRAMPLAYER
#include "ustd_array.h"
#include "timeout.h"
#endif

#include "muwerk.h"
#include "scheduler.h"

namespace ustd {

class MuppletDisplay {
  public:
    static const char *formatTokens[];

#ifdef USTD_FEATURE_PROGRAMPLAYER
    static const char *modeTokens[];

    /// Program Item Display Mode
    enum Mode {
        Left,     ///< Static left formatted text
        Center,   ///< Static centered text
        Right,    ///< Static right formatted text
        SlideIn,  ///< Text slides in char by char to the left side
    };
#endif

  protected:
    // font helper
    typedef struct {
        uint8_t baseLine;
        uint8_t xAdvance;
        uint8_t yAdvance;
        uint8_t dummy;
    } FontSize;

#ifdef USTD_FEATURE_PROGRAMPLAYER
    // program item
    typedef struct {
        String name;
        Mode mode;
        ustd::timeout duration;
        int16_t repeat;
        uint8_t speed;
        uint8_t font;
        uint16_t color;
        uint16_t bg;
        String content;
    } ProgramItem;

    // program item state
    enum ProgramState { None, FadeIn, Wait, FadeOut, Finished };
#endif

    // muwerk task management
    Scheduler *pSched;
    int tID;

    // device configuration
    String name;

    // runtime
    uint8_t current_font;
    uint16_t current_bg;
    uint16_t current_fg;

#ifdef USTD_FEATURE_PROGRAMPLAYER
    // runtime - program control
    ustd::array<ProgramItem> program;
    ProgramItem default_item;
    int16_t program_counter;
    ProgramState program_state;
    int16_t program_pos;
    int16_t program_width;
    uint8_t program_height;
    unsigned long anonymous_counter;
    // runtime - effect control
    uint8_t delayCtr;   // effect delay counter
    uint16_t charPos;   // index of char to slide
    uint16_t lastPos;   // target position of sliding char
    uint16_t slidePos;  // position of sliding char
    uint8_t charX;      // width of current char
    uint8_t charY;      // height of current char
#endif

  public:
    MuppletDisplay(String name) : name(name) {
        current_font = 0;
        current_bg = 0;
        current_fg = 1;
    }

#ifdef USTD_FEATURE_PROGRAMPLAYER
    /*! Configures the program player
     * @param posY      The Y position where the program player starts (default: -1)
     * @param height    The effective height of the progarm player (default: 0)
     */
    void setPlayer(int16_t posY = -1, uint8_t height = 0) {
        int16_t w, h;
        getDimensions(w, h);
        if (posY < 0 || height == 0) {
            // player will be disabled - clear player area before disabling
            displayClear(0, program_pos, w, program_height);
        }
        if (height > h) {
            height = h;
        }
        if (posY > (h - height)) {
            posY = h - height;
        }
        program_pos = posY;
        program_width = w;
        program_height = height;
        if (height > 0) {
            // player will be enabled - clear player area
            displayClear(0, program_pos, w, program_height);
        }
    }

    /*! Set the internal default values for program items
     * @param mode      Display mode of the program item
     * @param duration  Duration in milliseconds of the program item
     * @param repeat    Number of repetitions for the program item. Set `0` for infinite repetitions
     * @param speed     Effect speed of the program item. Value range: 1 (slow) to 16 (fast)
     * @param font      Font index of program item. Set `0` for builtin font.
     * @param color     The color of the displayed item
     */
    void setDefaults(Mode mode, unsigned long duration, int16_t repeat, uint8_t speed, uint8_t font,
                     uint16_t color = 65535) {
        default_item.mode = mode;
        default_item.duration = duration;
        default_item.repeat = repeat;
        default_item.speed = speed > 16 ? 16 : speed;
        default_item.font = font < getTextFontCount() ? font : 0;
        default_item.color = color;
    }

    /*! Remove all program items
     */
    void clearItems() {
        program.erase();
        program_counter = 0;
        program_state = None;
        displayClear(0, program_pos, program_width, program_height);
    }
#endif

  protected:
#ifdef USTD_FEATURE_PROGRAMPLAYER
    void programInit() {
        // initialize default values
        default_item.mode = Left;
        default_item.duration = 2000;
        default_item.repeat = 1;
        default_item.speed = 16;
        default_item.font = 0;
        default_item.color = 65535;
        default_item.bg = 0;

        // initialize state machine
        program_pos = -1;
        program_width = 0;
        program_height = 0;
        program_counter = 0;
        program_state = None;
        anonymous_counter = 0;
    }

    void programLoop() {
        if (program.length() == 0 || program_height == 0 || program_pos < 0) {
            return;
        }
        // save state
        int16_t x, y, w, h;
        bool cur_wrap = getTextWrap();
        uint8_t cur_font = current_font;
        getCursor(x, y);
        getDimensions(w, h);

        if (program_state == None) {
            startProgramItem(program[program_counter], x, y, w, h);
        }
        if (program_state == FadeIn) {
            fadeInProgramItem(program[program_counter], x, y, w, h);
        }
        if (program_state == Wait) {
            waitProgramItem(program[program_counter], x, y, w, h);
        }
        if (program_state == FadeOut) {
            fadeOutProgramItem(program[program_counter], x, y, w, h);
        }
        if (program_state == Finished) {
            endProgramItem(program[program_counter], x, y, w, h);
        }
        if (program.length() == 0) {
            displayClear(0, program_pos, program_width, program_height);
        }

        // restore state
        setTextFont(cur_font, 0);
        setTextWrap(cur_wrap);
        setCursor(x, y);
    }
#endif

    virtual bool commandParser(String command, String args, String topic) {
        if (command.startsWith("cmnd/")) {
            return commandCmdParser(command.substring(5), args);
        } else if (command.startsWith("cursor/")) {
            return cursorParser(command.substring(7), args, topic + "/cursor");
        } else if (command.startsWith("wrap/")) {
            return wrapParser(command.substring(5), args, topic + "/wrap");
#ifdef USTD_FEATURE_PROGRAMPLAYER
        } else if (command == "count/get") {
            return publishItemsCount();
        } else if (command.startsWith("default/")) {
            return commandDefaultParser(command.substring(8), args, topic + "/default");
        } else if (command.startsWith("items/")) {
            return commandItemsParser(command.substring(6), args, topic + "/items");
        } else if (command.startsWith("content/")) {
            return commandContentParser(command.substring(8), args, topic + "/content");
#endif
        }
        return false;
    }

    virtual bool commandCmdParser(String command, String args) {
        int16_t x, y, w, h, d, width, height;
        FontSize fs;
        array<String> params;
        if (command == "clear") {
            if (args.length()) {
                split(args, ';', params);
            }
            getDimensions(width, height);
            fs = getTextFontSize();
            switch (params.length()) {
            case 0:
                // clear the whole screen
                displayClear(0, 0, width, height);
                setCursor(0, 0 + fs.baseLine);
                return true;
            case 1:
                // clear the specified line
                y = parseLong(params[0], 0);
                displayClear(0, y * fs.yAdvance, width, fs.yAdvance);
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
            h = parseToken(shift(args, ';', "left"), formatTokens);
            if (h == 3) {
                // float mode
                String size = shift(args, ';');
                w = parseLong(shift(size, '.', ""), width);
                d = parseRangedLong(size, 0, w, 0, w);
                if (args.length() == 0) {
                    // empty value - blank out area
                    fs = getTextFontSize();
                    displayClear(x, y, w, fs.yAdvance);
                } else if (!isNumber(args.c_str())) {
                    // not a Number
                    displayError(x, y, w, 2);
                } else {
                    args = String(atof(args.c_str()), d);
                    if (!displayFormat(x, y, w, 2, args, current_font, current_fg, current_bg)) {
                        // overflow
                        displayError(x, y, w, 2);
                    }
                    return true;
                }
                return true;
            }
            w = parseLong(shift(args, ';', ""), width);
            displayFormat(x, y, w, h, args, current_font, current_fg, current_bg);
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

#ifdef USTD_FEATURE_PROGRAMPLAYER
    bool commandDefaultParser(String command, String args, String topic) {
        if (command == "get") {
            return publishDefaults(topic);
        } else if (command == "set") {
            if (parseDefaults(args)) {
                return publishDefaults(topic);
            }
        } else if (command == "mode/get") {
            return publishDefaultMode(topic + "/mode");
        } else if (command == "mode/set") {
            if (parseMode(args, default_item)) {
                return publishDefaultMode(topic + "/mode");
            }
        } else if (command == "repeat/get") {
            return publishDefaultRepeat(topic + "/repeat");
        } else if (command == "repeat/set") {
            if (parseRepeat(args, default_item)) {
                return publishDefaultRepeat(topic + "/repeat");
            }
        } else if (command == "duration/get") {
            return publishDefaultDuration(topic + "/duration");
        } else if (command == "duration/set") {
            if (parseDuration(args, default_item)) {
                return publishDefaultDuration(topic + "/duration");
            }
        } else if (command == "speed/get") {
            return publishDefaultSpeed(topic + "/speed");
        } else if (command == "speed/set") {
            if (parseSpeed(args, default_item)) {
                return publishDefaultSpeed(topic + "/speed");
            }
        } else if (command == "font/get") {
            return publishDefaultFont(topic + "/font");
        } else if (command == "font/set") {
            if (parseFont(args, default_item)) {
                return publishDefaultFont(topic + "/font");
            }
        }
        return false;
    }

    bool commandItemsParser(String command, String args, String topic) {
        String name, operation;
        if (command == "clear") {
            clearItems();
            return publishItemsCount();
        } else if (command == "get") {
            return publishItems(topic);
        } else if (command == "add") {
            addItem("unnamed_" + String(++anonymous_counter), args);
            return publishItemsCount();
        } else if (parseItemCommand(command, name, operation)) {
            int16_t index = findItemByName(name.c_str());
            if (operation == "set") {
                if (index < 0) {
                    index = addItem(name, args);
                } else {
                    index = replaceItem(index, args);
                }
                if (index >= 0) {
                    return publishItem(topic, index);
                }
            } else if (operation == "get") {
                return publishItem(topic, index);
            } else if (operation == "jump") {
                if (jumpItem(index)) {
                    return publishItem(topic, index);
                }
            } else if (operation == "clear") {
                if (clearItem(index)) {
                    return publishItemsCount();
                }
            }
        }
        return false;
    }

    bool commandContentParser(String command, String args, String topic) {
        String name, operation;
        if (command == "clear") {
            clearItems();
            return publishItemsCount();
        } else if (command == "get") {
            return publishContents(topic);
        } else if (command == "add") {
            addContent("unnamed_" + String(++anonymous_counter), args);
            return publishItemsCount();
        } else if (parseItemCommand(command, name, operation)) {
            int16_t index = findItemByName(name.c_str());
            if (operation == "set") {
                if (index < 0) {
                    index = addContent(name, args);
                } else {
                    index = replaceContent(index, args);
                }
                if (index >= 0) {
                    return publishContent(topic, index);
                }
            } else if (operation == "get") {
                return publishContent(topic, index);
            } else if (operation == "jump") {
                if (jumpItem(index)) {
                    return publishContent(topic, index);
                }
            } else if (operation == "clear") {
                if (clearItem(index)) {
                    return publishItemsCount();
                }
            }
        }
        return false;
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

    bool publishItemsCount() {
        pSched->publish(name + "/display/count", String(program.length()));
        return true;
    }

    bool parseDefaults(String args) {
        bool changed = parseMode(shift(args, ';'), default_item);
        changed = changed || parseRepeat(shift(args, ';'), default_item);
        changed = changed || parseDuration(shift(args, ';'), default_item);
        changed = changed || parseSpeed(shift(args, ';'), default_item);
        changed = changed || parseFont(shift(args, ';'), default_item);
        return changed;
    }

    bool publishDefaults(String topic) {
        pSched->publish(topic, getItemString(default_item));
        return true;
    }

    bool parseMode(String args, ProgramItem &item) {
        int16_t iMode = parseToken(args, modeTokens);
        if (iMode >= 0 && iMode != item.mode) {
            item.mode = (Mode)iMode;
            return true;
        }
        return false;
    }

    bool publishDefaultMode(String topic) {
        pSched->publish(topic + "/mode", modeTokens[default_item.mode]);
        return true;
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

    bool publishDefaultRepeat(String topic) {
        pSched->publish(topic + "/repeat", String(default_item.repeat));
        return true;
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

    bool publishDefaultDuration(String topic) {
        pSched->publish(topic + "/duration", String(default_item.duration));
        return true;
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

    bool publishDefaultSpeed(String topic) {
        pSched->publish(topic + "/speed", String(default_item.speed));
        return true;
    }

    bool parseFont(String args, ProgramItem &item) {
        if (args.length()) {
            int value = atoi(args.c_str());
            if (value >= 0 && value < (int)getTextFontCount() && value != item.font) {
                item.font = value;
                return true;
            }
        }
        return false;
    }

    bool publishDefaultFont(String topic) {
        pSched->publish(topic + "/font", String(default_item.font));
        return true;
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
            displayClear(0, program_pos, program_width, program_height);
        }
        return true;
    }

    bool publishItem(String topic, int16_t i) {
        if (i < 0 || i > (int16_t)program.length() - 1) {
            return true;
        }
        pSched->publish(topic + "/" + program[i].name, getItemString(program[i]));
        return true;
    }

    bool publishItems(String topic) {
        for (unsigned int i = 0; i < program.length(); i++) {
            pSched->publish(topic + "/" + program[i].name, getItemString(program[i]));
        }
        return true;
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

    bool publishContent(String topic, int16_t i) {
        if (i < 0 || i > (int16_t)program.length() - 1) {
            return true;
        }
        pSched->publish(topic + "/" + program[i].name, program[i].content);
        return true;
    }

    bool publishContents(String topic) {
        for (unsigned int i = 0; i < program.length(); i++) {
            pSched->publish(topic + "/" + program[i].name, program[i].content);
        }
        return true;
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
#endif

    // can be removed with release of mupplet-core 0.4.1
    bool isNumber(const char *value, bool integer = false) {
        if (!value) {
            return false;
        }
        if (*value == '-') {
            value++;
        }
        bool decimalpoint = false;
        while (*value) {
            if (*value < '0' || *value > '9') {
                if (integer || decimalpoint || *value != '.') {
                    return false;
                } else {
                    decimalpoint = true;
                }
            }
            value++;
        }
        return true;
    }

    void displayError(int16_t x, int16_t y, int16_t w, int16_t align) {
        if (w >= 5) {
            displayFormat(x, y, w, align, "Error", current_font, current_fg, current_bg);
        } else if (w >= 3) {
            displayFormat(x, y, w, align, "Err", current_font, current_fg, current_bg);
        } else {
            displayFormat(x, y, w, align, "E", current_font, current_fg, current_bg);
        }
    }

    // abstract methods
    virtual void getDimensions(int16_t &width, int16_t &height) = 0;
    virtual bool getTextWrap() = 0;
    virtual void setTextWrap(bool wrap) = 0;
    virtual FontSize getTextFontSize() = 0;
    virtual uint8_t getTextFontCount() = 0;
    virtual void setTextFont(uint8_t font, int16_t baseLineAdjustment) = 0;
    virtual void getCursor(int16_t &x, int16_t &y) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    virtual void displayClear(int16_t x, int16_t y, int16_t w, int16_t h) = 0;
    virtual void displayClear(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bg) = 0;
    virtual void displayPrint(String content, bool ln = false) = 0;
    virtual bool displayFormat(int16_t x, int16_t y, int16_t w, int16_t align, String content,
                               uint8_t font, uint16_t color, uint16_t bg) = 0;

#ifdef USTD_FEATURE_PROGRAMPLAYER
    virtual void changedProgramItem(ProgramItem &item) {
        switch (item.mode) {
        case Left:
            displayFormat(0, program_pos, program_width, 0, item.content, item.font, item.color,
                          item.bg);
            break;
        case Center:
            displayFormat(0, program_pos, program_width, 1, item.content, item.font, item.color,
                          item.bg);
            break;
        case Right:
            displayFormat(0, program_pos, program_width, 2, item.content, item.font, item.color,
                          item.bg);
            break;
        case SlideIn:
            if (program_state == FadeIn && charPos < item.content.length() - 1) {
                int16_t x, y;
                displayFormat(0, program_pos, program_width, 0, item.content.substring(0, charPos),
                              item.font, item.color, item.bg);
                getCursor(x, y);
                lastPos = x;
                initNextCharDimensions(item);
            } else {
                displayFormat(0, program_pos, program_width, 0, item.content, item.font, item.color,
                              item.bg);
                program_state = Wait;
            }
            break;
        default:
            break;
        }
    }

    virtual void startProgramItem(ProgramItem &item, int16_t x, int16_t y, int16_t w, int16_t h) {
        item.duration.reset();
        switch (item.mode) {
        case Left:
            displayFormat(0, program_pos, program_width, 0, item.content, item.font, item.color,
                          item.bg);
            program_state = Wait;
            break;
        case Center:
            displayFormat(0, program_pos, program_width, 1, item.content, item.font, item.color,
                          item.bg);
            program_state = Wait;
            break;
        case Right:
            displayFormat(0, program_pos, program_width, 2, item.content, item.font, item.color,
                          item.bg);
            program_state = Wait;
            break;
        case SlideIn:
            charPos = 0;
            lastPos = 0;
            delayCtr = 17 - item.speed;
            slidePos = w;
            if (initNextCharDimensions(item)) {
                displayClear(0, program_pos, program_width, program_height);
                program_state = FadeIn;
            } else {
                displayFormat(0, program_pos, program_width, 0, item.content, item.font, item.color,
                              item.bg);
                program_state = Wait;
            }
            break;
        default:
            program_state = Finished;
            break;
        }
    }

    virtual void fadeInProgramItem(ProgramItem &item, int16_t x, int16_t y, int16_t w, int16_t h) {
        if (item.mode == SlideIn) {
            if (--delayCtr) {
                return;
            }
            delayCtr = 17 - item.speed;
            slidePos--;
            displayFormat(slidePos, program_pos, program_width - slidePos, 0,
                          item.content.substring(charPos, charPos + 1), item.font, item.color,
                          item.bg);

            // prepare for next iterations:
            if (slidePos <= lastPos) {
                // char has arrived
                lastPos += charX;
                slidePos = w;
                if (lastPos >= slidePos) {
                    // display full
                    fadeInEnd(item, x, y, w, h);
                    return;
                }
                ++charPos;
                if (!initNextCharDimensions(item)) {
                    // end of string
                    fadeInEnd(item, x, y, w, h);
                    return;
                }
            }
        } else {
            fadeInEnd(item, x, y, w, h);
            return;
        }
    }

    virtual void fadeInEnd(ProgramItem &item, int16_t x, int16_t y, int16_t w, int16_t h) {
        item.duration.reset();
        program_state = Wait;
    }

    virtual void waitProgramItem(ProgramItem &item, int16_t x, int16_t y, int16_t w, int16_t h) {
        if (item.duration.test()) {
            program_state = FadeOut;
        }
    }

    virtual void fadeOutProgramItem(ProgramItem &item, int16_t x, int16_t y, int16_t w, int16_t h) {
        // switch (item.mode) {
        // case Left:
        // case Center:
        // case Right:
        // case SlideIn:
        // default:
        // }
        fadeOutEnd(item, x, y, w, h);
    }

    virtual void fadeOutEnd(ProgramItem &item, int16_t x, int16_t y, int16_t w, int16_t h) {
        program_state = Finished;
    }

    virtual void endProgramItem(ProgramItem &item, int16_t x, int16_t y, int16_t w, int16_t h) {
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

    virtual bool initNextCharDimensions(ProgramItem &item) = 0;

#endif
};

const char *MuppletDisplay::formatTokens[] = {"left", "center", "right", "number", nullptr};

#ifdef USTD_FEATURE_PROGRAMPLAYER
const char *MuppletDisplay::modeTokens[] = {"left", "center", "right", "slidein", nullptr};
#endif
}  // namespace ustd