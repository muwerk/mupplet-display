[![PlatformIO CI][image_CI]][badge_CI] [![Dev Docs][image_DOC]][badge_DOC]

**Note:** This project, while being released, is still WIP. Additional functionality from
[Research-mupplets](https://github.com/muwerk/Research-mupplets) is being refactored and
ported into this project.

Also the interfaces and the MPI (Message Programming Interface) of the display mupplets
is still not consolidated and subject to constant changes!

mupplet-display
===============

**mu**werk a**pplet**s; mupplets: functional units that support specific hardware or reusable
applications.

**mupplets** use muwerks MQTT-style messaging to pass information between each other on the
same device. If connected to an MQTT-server via munet, all functionallity is externally
available through an MQTT server such as Mosquitto unless filtered by specific rules.

The `mupplet-display` library consists of the following modules:

* [DisplayDigitsMAX72XX][DisplayDigitsMAX72XX_DOC]
  The `DisplayDigitsMAX72XX` mupplet implements a display server for 7 segment digit modules driven
  by a MAX7219 or MAX7221 connected via SPI. See
  [DisplayDigitsMAX72XX Application Notes][DisplayDigitsMAX72XX_NOTES]
* [DisplayMatrixMAX72XX][DisplayMatrixMAX72XX_DOC]
  The `DisplayMatrixMAX72XX` mupplet implements a single line display server based on multiple 8x8
  led matrix modules driven by a MAX7219 or MAX7221 connected via SPI. See
  [DisplayMatrixMAX72XX Application Notes][DisplayMatrixMAX72XX_NOTES]
* [DisplayMatrixST7735][DisplayMatrixST7735_DOC]
  The `DisplayDigitsMAX72XX` mupplet implements a display server for color TFT displays driven by a
  Sitronix ST7735 color single-chip TFT controller. See
  [DisplayMatrixST7735 Application Notes][DisplayMatrixST7735_NOTES]



Dependencies
------------

* Note: **All** mupplets require the libraries [ustd][gh_ustd], [muwerk][gh_muwerk] and
 [mupplet-core][gh_mupcore]

For ESP8266 and ESP32, it is recommended to use [munet][gh_munet] for network connectivity.

### Additional hardware-dependent libraries ###

Note: third-party libraries may be subject to different licensing conditions.

Mupplet                     | Type   | Luminosity | Chainable | Templates | Presenter | Colors | Hardware | Dependencies
--------------------------- | ------ | ---------- | --------- | --------- | --------- | ------:| -------- | -----------------------
`display_digits_max72xx.h`  | Digits | Dimmable   | Up to 16  |           |           |      1 | Seven segment led digit modules (8 digits) driven by a MAX7219 or MAX7221 | SPI
`display_matrix_max72xx.h`  | Matrix | Dimmable   | Up to 16  |           |           |      1 | 8x8 led matrix modules driven by a MAX7219 or MAX7221 | [Adafruit GFX Library][2], [Adafruit BusIO][1], Wire, SPI
`display_matrix_st7735.h`   | Matrix | Backlight  | No        |           |           |  65536 | Various TFT display modules driven by a ST7735 | [Adafruit ST7735 and ST7789 Library][3], [Adafruit GFX Library][2], [Adafruit BusIO][1], Wire, SPI


Message Programming Interface
-----------------------------

All messages always start with the `<mupplet-name>` as first topic part. Therefore all subsequent
documentation does not mention this part.

### Display Luminosity ###

All light emitting or backlit displays have also a luminosity aspect (on, off, blightness) that
can be controlled with the standard light messages:

#### Light Related Messages sent by Mupplet ####

Topic                  | Message Body                    | Description
---------------------- | ------------------------------- | ---------------------------------------------------------------------
`light/unitbrightness` | normalized brightness [0.0-1.0] | `0.34`: Float value encoded as string. Not sent on automatic changes (e.g. pulse mode)
`light/state`          | `on` or `off`                   | Current light state (`on` is not sent on pwm intermediate values)

#### Light Related Messages received by Mupplet ####

Topic               | Message Body                    | Description
------------------- | ------------------------------- | ---------------------------------------------------------------------
`light/set`         | `on`, `off`, `true`, `false`, `pct 34`, `34%`, `0.34` | Light can be set fully on or off with on/true and off/false. A fractional brightness of 0.34 (within interval [0.0, 1.0]) can be sent as either `pct 34`, or `0.34`, or `34%`.
`light/mode/set`    | `passive`, `pulse <duration_ms>`, `blink <intervall_ms>[,<phase-shift>]`, `pattern <pattern>[,<intervall>[,<phase>]]` or `wave <intervall_ms>[,<phase-shift>]` | Mode passive does no automatic light state changes, `pulse` switches the light on for `<duration_ms>` ms, then light goes back to passive mode. `blink` changes the light state very `interval_ms` on/off, `wave` uses pwm to for soft changes between on and off states. Optional comma-speratated phase [0.0, ..., 1.0] can be added as a phase-shift. Two lights, one with `wave 1000` and one with `wave 1000,0.5` blink inverse. Patterns can be specified as string containing `+`,`-`,`0`..`9` or `r`. `+` is light on during `<intervall>` ms, `-` is off, `0`..`9` brightness-level. An `r` at the end of the pattern repeats the pattern. `"pattern +-+-+-+++-+++-+++-+-+-+---r,100"` lets the board signal SOS.

#### Example ####

To switch the display on, the following message will be sent:

````
<mupplet-name>/light/set    on
````

The mupplet switches the illumination on at 100% and responds with:

````
<mupplet-name>/light/state          on
<mupplet-name>/light/unitbrightness 1.000
````

### Display Coordinate System ###

The display coordinate system depends upon the display type but always starts at the upper left
corner. In digit displays, a coordinate describes the position of a specific digit. The
coordinate x=0,y=0 refers to the first digit of a display. In matrix displays, a coordinate
describes the position of a specific pixel.

### Display Font Support ###

All matrix display mupplets are built using Adafruit's GFX library. Therefore all compatible GFX
fonts can be used. There is always a built in font available at index number 0. Additional fonts
can be added to the muplet and are addressed by the index number (order of addition):

````
#include "muMatrix8ptRegular.h"
#include "muHeavy8ptBold.h"

    ...

    // initialize display
    display.begin(&sched, true);
    display.addfont(&muMatrix8ptRegular, 7);  // font index number 1
    display.addfont(&muMatrix8ptBold, 7);     // font index number 2
````

### Basic Display Control ###

The following properties can be set or retrieved:

* `display/cursor` - The cursor position, values separated by semicolon - e.g. `<x>;<y>`
* `display/x/cursor` - The x cursor position
* `display/y/cursor` - The y cursor position
* `display/wrap` - The text wrap mode (`on`, `off`, `true`, `false`,)
* `display/font` - The current font index (numerical value starting at 0)
* `display/color` - The current text color
* `display/background` - The current text background

For color-capable displays, colors are represented as unsigned 16-bit values. The primary color
components — red, green and blue — are all “packed” into a single 16-bit variable, with the most
significant 5 bits conveying red, middle 6 bits conveying green, and least significant 5 bits
conveying blue. That extra bit is assigned to green because our eyes are most sensitive to green
light.

Colors are always returned as a hexadecimal number in `0xdddd` notation. This notation must also
be used when specifying colors in addition to the following predefined names:

Color Name  | Value
----------- | ----------------------------
`black`     | `0x0000`
`white`     | `0xffff`
`red`       | `0xf800`
`green`     | `0x07e0`
`blue`      | `0x001f`
`cyan`      | `0x07ff`
`magenta`   | `0xf81f`
`yellow`    | `0xffe0`
`orange`    | `0xfc00`


Examples:
````
SEND> <mupplet-name>/display/cursor/get
RECV> <mupplet-name>/display/cursor 0;7

SEND> <mupplet-name>/display/font/set 1
RECV> <mupplet-name>/display/font 1

SEND> <mupplet-name>/display/color/set orange
RECV> <mupplet-name>/display/color 0xfc00

SEND> <mupplet-name>/display/backgroun/set 0x0000
RECV> <mupplet-name>/display/color 0x0
````

The following commands can be issued:

#### clear ####

This command clears the whole or a portion of the display. In case of matrix displays, this means
resettings the color of the affected portion to the text background color.

````
SEND> <mupplet-name>/display/cmnd/clear
````
Without any parameter, the command clears the whole display.

````
SEND> <mupplet-name>/display/cmnd/clear <linenum>
````
By specifying a line number, only the specified line number is cleared. In matrix displays the area
defined by a "line number" depends upon the selected font.

````
SEND> <mupplet-name>/display/cmnd/clear <x>;<y>
````
By specifying an x and y coordinate, the rectangular area starting from the specified coordinate
up to the lower right edge of the display is cleared.

````
SEND> <mupplet-name>/display/cmnd/clear <x>;<y>;<w>;<h>
````
By specifying an x and y coordinate and a width and height, a corresponding rectangular area
starting from the specified coordinate with the specified width and height is cleared.

#### print and println ####

These two commands print the spcified content starting at the current cursor position. If wrap mode
is on, the string continues on the next line if it would be longer than the display width. After
the end of the operation, the cursor is positioned behind the last character of the string. The
special version `println` automatically advances to the next line after printing the string.

**Note:** When using these commands, the cursor always starts at the **baseline** position.

````
SEND> <mupplet-name>/display/cmnd/print <content>
````
Prints the <content> at the current cursor position.

````
SEND> <mupplet-name>/display/cmnd/println <content>
````
Prints the <content> at the current cursor position and sets the cursor to the next line beginning.

#### printat ####

This command prints the specified content at an exact position. If wrap mode is on, the string
continues on the next line if it would be longer than the display width. After the end of the
operation, the cursor is positioned behind the last character of the string.

````
SEND> <mupplet-name>/display/cmnd/printat <x>;<y>;<content>
````
Prints <content> starting at <x>,<y>

#### format ####

TBD...


The builtin default values are:
Value Name  | Value
----------- | ----------------------------
`x`         | `0`
`y`         | `0`
`mode`      | `left`
`width`     | Width of the display



````
SEND> <mupplet-name>/display/cmnd/format <x>;<y>;<align>;<len>;<content>
````

### Template Display Function ###

TBD....

### Presentation Server Function ###

TBD....




History
-------

- 0.1.0 (2021-02-21) Single Line Matrix Display Module based on MAX72XX driver

More mupplet libraries
----------------------

- [mupplet-core][gh_mupcore] microWerk Core mupplets
- [mupplet-sensor][gh_mupsensor] microWerk Sensor mupplets

References
----------

- [ustd][gh_ustd] microWerk standard library
- [muwerk][gh_muwerk] microWerk scheduler
- [munet][gh_munet] microWerk networking


[badge_CI]: https://github.com/muwerk/mupplet-display/actions
[image_CI]: https://github.com/muwerk/mupplet-display/workflows/PlatformIO%20CI/badge.svg
[badge_DOC]: https://muwerk.github.io/mupplet-display/docs/index.html
[image_DOC]: https://img.shields.io/badge/docs-dev-blue.svg

[DisplayDigitsMAX72XX_DOC]: https://www.lipsum.com/
[DisplayDigitsMAX72XX_NOTES]: https://www.lipsum.com/
[DisplayMatrixMAX72XX_DOC]: https://muwerk.github.io/mupplet-display/docs/classustd_1_1DisplayMatrixMAX72XX.html
[DisplayMatrixMAX72XX_NOTES]: https://github.com/muwerk/mupplet-display/blob/master/extras/display-matrix-notes.md
[DisplayMatrixST7735_DOC]: https://www.lipsum.com/
[DisplayMatrixST7735_NOTES]: https://www.lipsum.com/

[gh_ustd]: https://github.com/muwerk/ustd
[gh_muwerk]: https://github.com/muwerk/muwerk
[gh_munet]: https://github.com/muwerk/munet
[gh_mufonts]: https://github.com/muwerk/mufonts
[gh_mupcore]: https://github.com/muwerk/mupplet-core
[gh_mupdisplay]: https://github.com/muwerk/mupplet-display
[gh_mupsensor]: https://github.com/muwerk/mupplet-sendsor

[1]: https://github.com/adafruit/Adafruit_BusIO
[2]: https://github.com/adafruit/Adafruit-GFX-Library
[3]: https://github.com/adafruit/Adafruit-ST7735-Library
[9]: https://github.com/marecl/HPDL1414

[HPDL1414 Arduino Library][9]