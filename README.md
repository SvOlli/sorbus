The Sorbus Computer
===================

The attempt to build a very simple 65C02 based computer by utilizing a
Raspberry Pi Pico clone for the rest of the functionality.

![Sorbus assembled](doc/images/SorbusAssembled.jpg)
![Sorbus parts](doc/images/SorbusParts.jpg)

The Project is capabile to run as an [Apple Computer 1](doc/apple1.md)
as well as an own monitor command prompt (mcp) to learn about the 65C02
processor and it siblings.

The project is fully open source, licensend under GPL v3:
  - the PCB design is in the folder pcb
    - the Raspberry Pi Pico clone is found under the term "Purple PR2040"
      on AliExpress and other sites
  - the 65C02 source code to run on the target is at src/65c02
    - compiling requires the [cc65](https://cc65.github.io/), which
      needs to be preinstalled.
  - the source code for the Raspberry Pi Pico clone is at src/rp2040
    - compiling requires the
      [Pico-SDK](https://github.com/raspberrypi/pico-sdk) from the
      Raspberry Pi Foundation, which will be downloaded if not previously
      configured. However it's dependencies must also be preinstalled.

[An addtional page](https://xayax.net/sorbus/) has been setup to explain more
about the ideas of the system, as well as some usecases.

To access the RP2040 without super user rights, make sure that your user is
in the groups "plugdev" and "dialout". You also need to add a udev rule.

/etc/udev/rules.d/99-picotool.rules:
```
SUBSYSTEM=="usb", \
    ATTRS{idVendor}=="2e8a", \
    ATTRS{idProduct}=="0003", \
    MODE="660", \
    GROUP="plugdev"
SUBSYSTEM=="usb", \
    ATTRS{idVendor}=="2e8a", \
    ATTRS{idProduct}=="000a", \
    MODE="660", \
    GROUP="plugdev"
```

