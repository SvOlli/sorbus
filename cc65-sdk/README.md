CC65 SDK for the Sorbus JAM Core
================================

This is a very simple SDK to build C programs for the Sorbus Computer
Native Core.

It builds its own `sorbus.lib` and comes with a few examples.

If you like to build your own programs create a directory `owncode`.
Every file.c and file.s found there will be compiled to an SX4 executable.

The files from the `example` direcotry are already installed onto the
filesystem when building a release, and can be run using the filebrowser.

To transfer the data, you can use the tool `wozcat` provided in the
`tools` directory. Usage is:

```
wozcat 0x400 R >/dev/ttySorbus <program.sx4
```

The parameters are:
- 0x400: fix start address in memory (SX4=Sorbus eXecutable @ $0400)
- R: run program after transfer (optional)
- /dev/ttySorbus: tty the sorbus is connected to, typically /dev/ttyACM0
- program.sx4: the program to transfer

As a faster alternative, `timcat` has been added as well in the `tools`
directory. Usage is:

```
timcat >/dev/ttySorbus 0x400 program.sx4
```

Parameters are the same as above.

Have fun!
