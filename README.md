GEX core
========

This is the (mostly) independent core of the GEX platform. To port GEX to a new STM32 microcontroller,
add this as a submodule and implement platform specific configuration by following the example of another 
existing port. Some examples are in the template folder.

License
-------

- Original GEX source code is provided under the Mozilla Public License v2.0.
- The VFS driver is adapted from the DAPLink project and remains licensed under the Apache license v2.0.
- STM32 device drivers etc. provided by ST Microelectronics are licensed under the 3-clause BSD license.
