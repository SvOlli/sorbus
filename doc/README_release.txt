How To Install
--------------

On the RP2040 board just hold the BOOTSEL button while plugging in USB.
Then a mass storage device will show up. Copy the desired *.uf2 file to
the top level of this device. Then the system will reboot with the new
core installed.

Cores available are:
- Monitor Command Prompt: MCP
- Apple 1
- Native Core utilizing an internal flash drive

For how to handle these cores, take a look at https://xayax.net/sorbus/

These files are provided:

sorbus-computer-apple1.uf2:
The firmware for emulating an Apple 1.

sorbus-computer-mcp.uf2:
The firmware for the Monitor Command Prompt.

sorbus-computer-native_alpha.uf2:
The firmware for the Native system. This requires a separatly flashed
kernel image as well as a filesystem image.

sorbus-computer-native_kernel.uf2:
The kernel required for the Native system.

sorbus-computer-native_cpmfs.uf2:
The filesystem image containing a CP/M-65 installation for the Native
system.

sorbus-computer-native_alpha_picotoool.uf2:
This image contains all data for the Native system in one file: firmware,
kernel and filesystem image. Only works when using picotool for loading.

For uploading the native firmware on the Sorbus the following order is
suggested:
- sorbus-computer-native_cpmfs.uf2
- sorbus-computer-native_kernel.uf2
- sorbus-computer-native_alpha.uf2

