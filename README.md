GEX core
========

GEX is a general purpose input/output device that attaches to a PC or laptop.
A GEX board provides direct access to on-chip peripherals and low level drivers in the firmware.

GEX is designed for quick hardware prototyping, evaluation, and electrical measurements. 
The board can be interfaced from C or Python, with plans to support MATLAB and other languages.

A wireless connection is also planned, likely using ESP8266, SX1276 or NRF24L01+.

GEX is currently compatible with STM32F072. It will be ported to STM32F103 (bluepill) later,
and there is a good chance of a ESP32 port in the future.

This repository contains the core of all STM32 ports, while the chip-specific start-up routine, 
low level drivers etc. are all in separate projects that include this as a sub-module.

GEX's F072 port can be run on the F072 Discovery evaluation board.

Availability
------------

GEX is currently in open preview. Please wait with contributing and/or forking until the first stable release.
The core API can be considered stable, but there's no guarantee of this until the 1.0.0 release. Unit driver APIs
may undergo minor changes as they are tested in practical demos and bugs or usability problems are found.

License
-------

- Original GEX source code is provided under the Mozilla Public License v2.0.
- The VFS driver is adapted from the DAPLink project and remains licensed under the Apache license v2.0.
- STM32 device drivers etc. provided by ST Microelectronics are licensed under the 3-clause BSD license.
