// mupplet_display.h - muwerk mupplet display library

#pragma once

/*! \mainpage mupplet-display is a collection of display applets for the muwerk scheduler

\section Introduction

mupplet-display implements the following classes based on the cooperative scheduler muwerk:

* * \ref ustd::DisplayMatrixMAX72XX

Additionally there are implementation for the following hardware driver classes:

* * \ref ustd::Max72XX
* * \ref ustd::Max72xxMatrix

For an overview, see:
<a href="https://github.com/muwerk/mupplet-display/blob/master/README.md">mupplet-display readme</a>

Libraries are header-only and should work with any c++11 compiler and support any  platforms able
to connect to the specific hardware.

This library requires the libraries ustd, muwerk, mupplet-core and requires a
<a href="https://github.com/muwerk/ustd/blob/master/README.md">platform
define</a>.

\section Reference
* * <a href="https://github.com/muwerk/mupplet-display">mupplet-display github repository</a>

depends on:
* * <a href="https://github.com/muwerk/ustd">ustd github repository</a>
* * <a href="https://github.com/muwerk/muwerk">muwerk github repository</a>
* * <a href="https://github.com/muwerk/mupplet-core">mupplet-core github repository</a>

*/

//! \brief The muwerk namespace
namespace ustd {}  // namespace ustd
