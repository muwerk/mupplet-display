// mup_gfx_display.h - mupplet graphic display base class

#pragma once

#include "gfxfont.h"
#include "helper/mup_display.h"

namespace ustd {

class MuppletGfxDisplay : public MuppletDisplay {
  protected:
    // runtime
    static const GFXfont *default_font;
    array<const GFXfont *> fonts;
    array<FontSize> sizes;
    uint8_t current_font;

  public:
    MuppletGfxDisplay(String name) : MuppletDisplay(name), fonts(4, ARRAY_MAX_SIZE, 4) {
        FontSize default_size = {0, 6, 8, 0};
        fonts.add(default_font);
        sizes.add(default_size);
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

    /*! Select the current font to use for output
     * @param font The index number of the selected font. The built in font has the index number 0.
     */
    void setfont(uint8_t font) {
        if (font < fonts.length()) {
            int16_t oldBaseLine = current_font ? sizes[current_font].baseLine : 6;
            int16_t newBaseLine = font ? sizes[font].baseLine : 6;
            current_font = font;
            setTextFont(font, newBaseLine - oldBaseLine);
        }
    }

  protected:
    // implementation
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

    // abstract methods implementation
    virtual uint8_t getTextFont() {
        return current_font;
    }

    virtual FontSize getTextFontSize() {
        return sizes[current_font];
    }
};

const GFXfont *MuppletGfxDisplay::default_font = nullptr;

}  // namespace ustd