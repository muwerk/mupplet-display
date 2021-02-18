DisplayMatrixMAX72XX Mupplet
============================

The DisplayMatrixMAX72XX Mupplet allows to control a led matrix display based on multiple 8x8 led
matrix modules driven by a MAX7219 or MAX7221 connected via SPI.

The mupplet acts as an intelligent display server supporting various commands and scenarios.


Hardware Setup
--------------

TBD



General Function Model
----------------------

The display server acts like a heavy rotation radio station: there is one **program** that loops
forever. The program is a list of **program items**. Program items have the following attributes:
* `mode` - Item presentation mode mode (See _Presentaiton Modes_)
* `repeat` - How often the program item will be repeated, if `0`, the item will be repeated
             until it is manually removed
* `duration` - How long the item is displayed before switching to the next item
* `speed` - If the item is displayed with an animation effect, the speed of the animation
* `font` - The font used to display the item

Program items can be added, removed and managed via messages. Additionally the mupplet keeps a
set of default program item settings in order to simplify the specification of of program items.

### Presentation Modes ###

The follwing presentation modes are supported:

Mode        | Description
----------- | -------------------------------------------------------------------------------
`Left`      | Left formatted text without any fadin/fadeout animation
`Center`    | Center formatted text without any fadin/fadeout animation
`Right`     | Right formatted text without any fadin/fadeout animation
`SlideIn`   | Left formatted text with a fadin animation sliding in the characters from right to left

More presentation modes are in preparation....

Messaging Interface
-------------------

All messages always start with the <mupplet-name> as first topic part. Therefore all subsequent
documentation does not mention this part.

### Display Luminosity ###

Since the display has also a luminosity aspect (on, off, blightness) it can be controlled with the
standard light messages:

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

The mupplet switches the illumination on at 100% and reposnds with:

````
<mupplet-name>/light/state          on
<mupplet-name>/light/unitbrightness 1.000
````
### General Program Item Format ###

A program item can be represented in one string by separating it's parts with semicolons. When
submitting program items, it is possible to omit values. In this case the display default value
is taken. When replacing items, the default is the previous value instead allowing to change
only spcific values.

````
[<mode>];[<repeat>];[<duration>];[<speed>];[<font>][;<content>]
````

The value of `mode` is not case sensistive.

### Display Default Values ###

The display allows to set the default values program items. The availability of default values
simplifies specifying and submitting program items or content shortcuts.

The builtin default values are:
Value Name  | Value
----------- | ----------------------------
`mode`      | `left`
`repeat`    | `1`
`duration`  | `2000`
`speed`     | `16`
`font`      | `0`

#### Setting and Getting the Default Values ####

The communication follows the standard get/set pattern: in order to query a value, a message with
the following topic must be sent:

`<mupplet-name>/display/default/<value-name>/get`

The value is returned in a message with the topic:

`<mupplet-name>/display/default/<value-name>`

In order to set a value, a message with the following topic and the value in the body must be
sent:

`<mupplet-name>/display/default/<value-name>/set`

If the value was parsed correctly and accepted, a message containing the new value is returned with
 the following topic:

`<mupplet-name>/display/default/<value-name>`

Example:
````
SEND> <mupplet-name>/display/default/mode/get
RECV> <mupplet-name>/display/default/mode left

SEND> matrix/display/default/mode/set center
RECV> <mupplet-name>/display/default/mode center
````

By omitting the value name in the topic, it is possible to get or set the entire default settings:

Example:
````
SEND> <mupplet-name>/display/default/get
RECV> <mupplet-name>/display/default center;1;2000;16;0

SEND> <mupplet-name>/display/default/set right;;3000;;
RECV> <mupplet-name>/display/default right;1;3000;16;0
````

#### Setting and Getting Program Items ####

Also program items are managed following the standard get/set pattern. Program items are **named**.
This means, that they will be accessible by specifying their name. It is also possible to create
program items without specifying a name. In this case, a unique name will be created automatically
returned with the confirmation message.

By setting a program item, it will be created if it does not exist, or updated if it does exist.
If the program item is displayed whilst it is updated, the display operation for the item will
restart.

In order to create a new or modify an existing program item, a message with the following topic
and the value in the body must be sent:

`<mupplet-name>/display/items/<item-name>/set`

If the program item was parsed correctly and accepted, a message containing the resulting program
item is returned with the following topic:

`<mupplet-name>/display/items/<item-name>`

Example:
````
SEND> <mupplet-name>/display/items/clock/set center;0;6000;;2;17:52:05
RECV> <mupplet-name>/display/items/clock center;0;6000;16;2;17:52:05

SEND> <mupplet-name>/display/items/date/set center;0;2000;;2;Mon, June 27 2021
RECV> <mupplet-name>/display/items/date/set center;0;2000;16;2;Mon, June 27 2021
````

The following example sets two program items that will alternate indefinitively. The time value
will be shown for 6 seconds, the date value for two seconds.

Updating the values is quite simple:
````
SEND> <mupplet-name>/display/items/clock/set ;;;;;17:52:06
RECV> <mupplet-name>/display/items/clock center;0;6000;16;2;17:52:06
````

There is also a shortcut topic hierarchy that allows to operate only on contents:
````
SEND> <mupplet-name>/display/content/clock/set 17:52:06
RECV> <mupplet-name>/display/content/clock 17:52:06
````

It is also possible to create content using this topic hierarchy. In this case all program item
values are taken from the default settings.

#### Removing Program Items ####

Program items are automatically removed when they reach the specified repetition. If a program item
was specified with 0 as the repeat value, it will stay there forever until it is deleted manually.
This can be achieved by sending a message with the following topic:

`<mupplet-name>/display/items/<item-name>/clear`

On success the system returns the number of active items in the topic:

`<mupplet-name>/display/count`

#### Operating on all Program Items ####

##### Get iItem Count #####

The number of active items can be queried by sending a message with the topic:

`<mupplet-name>/display/count/get`

The current number of program items will be returned in the topic

`<mupplet-name>/display/count`

##### Delete all program items #####

All program items can be deleted by sending a message with the topic:

`<mupplet-name>/display/items/clear`

On success the system returns the number of active items in the topic:

`<mupplet-name>/display/count`

##### Get all Active Items #####

A list of all active progarm items can be get by seind a message with the topic:

`<mupplet-name>/display/items/get`

Each active program item will be returned in a message with the topic:

`<mupplet-name>/display/items/<item-name>`

Example:
````
SEND> <mupplet-name>/display/items/get
RECV> <mupplet-name>/display/items/clock center;0;6000;16;2;17:52:05
      <mupplet-name>/display/items/date center;0;2000;16;2;Mon, June 27 201
````

#### Add Item without specifying a name ####

An program item can be added without specifying a name by sending a message with the
following topic:

`<mupplet-name>/display/items/add`

If the program item was parsed correctly and accepted, a message containing the resulting program
item is returned with the following topic:

`<mupplet-name>/display/items/<item-name>`

Example:
````
SEND> <mupplet-name>/display/items/add slidein;1;6000;;2;Leopoldo!
RECV> <mupplet-name>/display/items/unnamed_7712 slidein;1;6000;16;2;Leopoldo!
`````

