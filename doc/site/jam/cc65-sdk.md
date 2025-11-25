
# CC65-SDK

The [cc65](https://cc65.github.io/) is a suite providing a full toolchain
for cross-developing for the different variants of the 6502 CPU family.

This SDK provides the absolute bare minimum required to run cc65 targeting
the Sorbus Computer JAM Core. To use it, cc65 needs to be installed on
the build system.

On top level running `make` will build the C runtime library and some
examples, as well as a host tool `timcat` which can be used to upload
new binaries into the Sorbus Computer:

```bash
timcat /dev/ttySorbus 0x400 program.sx4
```

* /dev/ttySorbus = device of Sorbus, typically /dev/ttyACM0 on Linux
* 0x400 = start in memory, will also be jumped to after upload
* program.sx4 = name of the binary to upload
