[![PlatformIO CI][image_CI]][badge_CI] [![Dev Docs][image_DOC]][badge_DOC]

**Note:** This project, while being released, is still WIP. Additional functionality from
[Research-mupplets](https://github.com/muwerk/Research-mupplets) is being refactored and
ported into this project.

mupplet-display
===============

**mu**werk a**pplet**s; mupplets: functional units that support specific hardware or reusable
applications.

**mupplets** use muwerks MQTT-style messaging to pass information between each other on the
same device. If connected to an MQTT-server via munet, all functionallity is externally
available through an MQTT server such as Mosquitto.

The `mupplet-display` library consists of the following modules:

* [DisplayMatrixMAX72XX][DisplayMatrixMAX72XX_DOC]
  The `DisplayMatrixMAX72XX` mupplet implements a single line display server based on multiple 8x8
  led matrix modules driven by a MAX7219 or MAX7221 connected via SPI. See
  [DisplayMatrixMAX72XX Application Notes][DisplayMatrixMAX72XX_NOTES]


Dependencies
------------

* Note: **All** mupplets require the libraries [ustd][gh_ustd], [muwerk][gh_muwerk] and
 [mupplet-core][gh_mupcore]

For ESP8266 and ESP32, it is recommended to use [munet][gh_munet] for network connectivity.

### Additional hardware-dependent libraries ###

Note: third-party libraries may be subject to different licensing conditions.

Mupplet                     | Function | Hardware | Dependencies
--------------------------- | -------- | -------- | ---------------
`display_matrix_max72xx.h`  | Single Line Matrix Display | 8x8 led matrix modules driven by a MAX7219 or MAX7221 | [Adafruit GFX Library][2], [Adafruit BusIO][1], Wire, SPI


History
-------

- 0.1.0 (2021-02-XX) (Not yet Released) Single Line Matrix Display Module based on MAX72XX driver

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

[DisplayMatrixMAX72XX_DOC]: https://muwerk.github.io/mupplet-display/docs/classustd_1_1DisplayMatrixMAX72XX.html
[DisplayMatrixMAX72XX_NOTES]: https://github.com/muwerk/mupplet-display/blob/master/extras/display-matrix-notes.md

[gh_ustd]: https://github.com/muwerk/ustd
[gh_muwerk]: https://github.com/muwerk/muwerk
[gh_munet]: https://github.com/muwerk/munet
[gh_mufonts]: https://github.com/muwerk/mufonts
[gh_mupcore]: https://github.com/muwerk/mupplet-core
[gh_mupdisplay]: https://github.com/muwerk/mupplet-display
[gh_mupsensor]: https://github.com/muwerk/mupplet-sendsor

[1]: https://github.com/adafruit/Adafruit_BusIO
[2]: https://github.com/adafruit/Adafruit-GFX-Library
