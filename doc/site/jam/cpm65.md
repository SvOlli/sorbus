
# CP/M 65 on Sorbus JAM

[CP/M 65](https://github.com/davidgiven/cpm65) is an operating system
implemented by David Given. The Sorbus JAM is a good platform running
CP/M 65, because it relies on a mass storage device, and the internal
drive of the Sorbus JAM is very fast, so starting a program from disk
takes almost no time.

Also since the CP/M filesystem can also be accessed by the kernel of
the Sorbus JAM as well, exchanging files between both worlds is very
easy.

Booting into CP/M 65 is done by selecting Bootblock "1" in the reset
menu.

The git repository contains only a binary dump of a successful build.
To build CP/M 65 and update the files within the repository run
`src/tools/external-cpm65.sh`. Note that this will require the
[llvm-mos-sdk](https://github.com/llvm-mos/llvm-mos-sdk), which can be
installed using the script `src/tools/external-llvm-mos-sdk.sh`.
