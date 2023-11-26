How To Install
--------------

On the RP2040 board just hold the BOOTSEL button while plugging in USB.
Then a mass storage device will show up. Copy the desired *.uf2 file to
the top level of this device. Then the system will reboot with the new
core installed.

Cores available are:
- Monitor Command Prompt: MCP
- Apple 1

In alpha state there also is a new native core. This core defines a new
custom computer. This requires two binary images to be flashed as well,
which are under heavy development:
- Sorbus Native Kernel
- Internal Drive Image
Because, these files are in an early stage, they are not part of this
distribution. This will change, once they've matured.
