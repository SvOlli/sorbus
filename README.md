The Sorbus Computer
===================

The attempt to build a very simple 65C02 based computer by utilizing a
Raspberry Pi Pico clone for the rest of the functionality.

![Sorbus assembled](doc/images/SorbusAssembled.jpg)
![Sorbus parts](doc/images/SorbusParts.jpg)

The project is fully open source:
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
      configured

