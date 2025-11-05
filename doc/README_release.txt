How To Install
--------------

On the RP2040 board just hold the BOOTSEL button while plugging in USB.
Then a mass storage device will show up. Copy the desired *.uf2 file to
the top level of this device. Then the system will reboot with the new
core installed.

More documentation is available in the docs/ directory.

Cores available are:
- Monitor Command Prompt: MCP
- Apple 1
- JAM (Just Another Machine) utilizing an internal flash drive

For how to handle these cores, take a look at https://sorbus.xayax.net/
A local copy is provided in the directory "docs/".

These files are provided:

sorbus-computer-apple1.uf2:
The firmware for emulating an Apple 1.

sorbus-computer-mcp.uf2:
The firmware for the Monitor Command Prompt.

sorbus-computer-jam_alpha.uf2:
The firmware for the Native system. This requires a separatly flashed
kernel image as well as a filesystem image.

sorbus-computer-jam_kernel.uf2:
The kernel required for the Native system.

sorbus-computer-jam_cpmfs.uf2:
The filesystem image containing a CP/M-65 installation for the Native
system.

sorbus-computer-jam_alpha_picotoool.uf2:
This image contains all data for the Native system in one file: firmware,
kernel and filesystem image. Only works when using picotool for loading.

For uploading the JAM firmware on the Sorbus the following order is
suggested:
- sorbus-computer-jam_cpmfs.uf2
- sorbus-computer-jam_kernel.uf2
- sorbus-computer-jam_alpha.uf2

sound_iotest.uf2:
Code for the Sorbus-Sound board: test the 65C02 bus communication

sound_mod.uf2:
Code for the Sorbus-Sound board: a mod player
WIP: right now just playing a song without 65C02 bus

sound_sd.uf2:
Code for the Sorbus-Sound board: SD card test program
WIP: no real code here right now

sound_sid.uf2:
Code for the Sorbus-Sound board: dual SID emulation
WIP: missing merge with sd and mod support

vga_iotest.uf2:
Code for the Sorbus-VGA board: test the 65C02 bus communication

vga_term.uf2:
Code for the Sorbus-VGA board: a hack to display a terminal console
WIP: communication on the 65C02 bus is still missing

fb32x32_iotest.uf2:
Code for the repurposed Chipset board: test the 65C02 bus communication

fb32x32_usbuart.uf2:
Code for the repurposed Chipset board: simuation via USB UART
WIP: there's no software capable of displaying the output yet

fb32x32_ws2812.uf2:
Code for the repurposed Chipset board: output data to WS2812.
The order of the LEDs is illustrated in doc/images/WS2812_order.gif

