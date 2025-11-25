
# Terminal Software

The primary interface for the Sorbus Computer is the USB UART. So for
communication so called "Terminal Software" is required that manages
the communication via a serial device.

Here are the results on trying out what was encountered so far.


## Linux

Main development is done on Linux. There are the terminal programs tested
and what's needed to know about them.


### microcom

Starting from command line:

```bash
microcom -p /dev/ttyACM0 -s 115200
```

Typically the setting of the baud rate is not required here. Quit can be done
by pressing `Ctrl+\` and entering "quit" at the prompt. However pressing
`Ctrl+\` twice gets the same job done and is faster.


### picocom

Starting from command line:

```bash
picocom -b 115200 /dev/ttyACM0
```

`Ctrl+A`, `Ctrl+X` closes connection.


### screen

Starting from command line:

```bash
screen /dev/ttyACM0
```

Works well, `Ctrl+A`, `K` to quit.


### minicom

Sadly minicom does not work with a couple of things the terminal output
requires.

- escape sequence for setting the cursor does not work for large numbers, which
  is required for screen size detection (a workaround for this is possible)
- UTF-8 output is broken in a way that output is counted in glyphs, not bytes,
  which breaks the output. This has been reported
  [in 2017](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=872051) and is
  not fixed as tested with the development version at the end of 2025.

Just don't use it, thanks.


## Windows

### PuTTY

Just works. Recommended. Click on "Connection Type: Serial" and enter the
correct port. You can use the device manager to figure that out. Speed is
115200.

[Download link](https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html).


### MobaXterm

It has been tested and works well with both the Home and the Professional
Edition. Does a lot more. Click on "Session", then select "Serial", the
correct "Serial port" from drop down list (huge advantage compared to PuTTY),
and set "Speed (bps)" to 115200. Portable version also available.
Favorite choice for Windows. 

[Download link](https://mobaxterm.mobatek.net/download.html).


### TeraTerm Pro

This is from a time where UTF-8 was not a topic at all.
Just fughettaboutit.


## MacOS

Most of the software for Linux should also work on MAC the same way, when
installed. The filename of the serial device will be significantly different.

### cu

This is a choice by an experienced MAC user.
