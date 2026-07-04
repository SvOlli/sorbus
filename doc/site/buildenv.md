
# Setting Up A Build Environment

## Installing The Bases

`make setup-apt-dev` will setup the development system with everything
required to build, transfer the firmware and run it.

This will run the following commands. Everything is prefixed with `sudo`.

```bash
apt update
apt install gdb-multiarch cmake gcc-arm-none-eabi libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib cc65 p7zip-full cpmtools build-essential \
    pkg-config libgd-dev mkdocs pkgconf libusb-1.0-0-dev microcom
gpasswd -a $USER dialout
gpasswd -a $USER plugdev
```

The $USER is added to the groups `dialout` and `plugdev`, so that it's
possible to use the terminal without sudo.

## Stow: An Elegant Way To Install Local Packages

Some software might not work very well in current versions, or it's not
available in apt based distros at all. For installing self-compile code
system-wide, there is a tool called stow. There are at least two
implementations available `gnu-stow` implemented in PERL, `nstow`
implemented in C. `gnu-stow` is available in Debian and Ubuntu. If you
want to use `nstow` instead, you can use the script provided.

To make easier use of this, there are a couple of shell scripts
implemented to help with setting this up.

Every script at start greets you with something like this (using
nstow-1.0 as an example):
```
This script downloads, compiles (when required) and installs
nstow-1.0

Download and compilation will be done in
/home/svolli/src/sorbus/local

Installation will be done into
/usr/local/stow/nstow-1.0
installation requires sudo

Links will be created in
/usr/local
need sudo to create links

Continue? 
```

It explains everything is going to. It names the package it's going to
install, which part of the filesystem it will use to download and compile,
where it will be installing the binaries, where it will be setting up
the links and for what steps it will be requiring sudo. And it will wait
for a confirmation there.

Also: please don't run those scripts as root, they will sudo when they
require to.

Uninstallation is easy (again using nstow-1.0 as an example):
```bash
cd /usr/local/stow
sudo stow -Dv nstow-1.0
sudo rm -rf nstow-1.0
```
That's it. You can use stow also to add your own packages. Instead of
installing to `/usr/local`, you install to
`/usr/local/stow/<your_package_name>`, and run
```bash
cd /usr/local/stow
sudo stow -v nstow-1.0
```

I've also seen a couple of tutorials on how to manage your ".dotfiles"
within git or alike and then have them symlinked to the correct places.


### src/tools/local-nstow.sh

This script downloads `nstow` from
[https://www.gnusto.com/](https://www.gnusto.com/), make small
adjustments, compiles it and installs it at `/usr/local/stow/nstow-1.0`.
From there it is then used to bootstrap itself.

Alternatively, you can use run `apt install stow` to install GNU stow,
which is written in perl. I perfer the C port, so that's why it's in
there. Also, to bootstrap, you'd need to run
`nstow-1.0/bin/stow nstow-1.0` since the executable is not in the $PATH.
The scripts takes care of this for you.

### src/tools/local-arm-none-eabi-toolchain.sh

Some change has been made to the toolchain so that code compiled on
Debian 13 (Trixie) runs significantly slower than the same code compiled
on Debian 12 (Bookworm). To compensate for that, the "best" precompiled
version is downloaded from the [ARM website](https://developer.arm.com/)
and installed.

### src/tools/local-picotool.sh

This gets a bit tough on the implementation. To figure out the version
number for displaying where to install to, it requires to check out the
picotool repository. So even before showing you the text what it's about
to do, it already downloads picotool via git.

And to build the picotool, it will also download the pico-sdk, unless
it's downloaded already. Also, for USB transfer, this requires the
usb-dev package to be installed, which is done by `make setup-apt-dev`.

### src/tools/local-exomizer.sh

exomizer is a tool for good 8-bit compression. The idea is to use the
powerful host machine for compression so that it can be decompressed good
on the target system.

However, distribution of this software package is a bit odd. Instead of
using the git repo of bitbucket, the source code is hosted as a zip
archive within a wiki set up using bitbucket.

The script tries to adjust for that as good as possible. Also this tool
is only needed if data needs to be compressed again due to changes. For
building the compressed data is checked in.

## Other Scripts Used For Development

There are also some other scripts I hacked together for making some
things a bit easier. Those I typically run from top level directory,
but they can be run from anywhere.

### src/tools/upload.sh

This it the script that I use to compile and upload. The default
configuration is to upload new firmware and the kernel ROM. To also
upload the internal drive image, you can use
`src/tools/upload.sh build/rp2040/jam_alpha_picotool.uf2`

### src/tools/mkdocs.sh

This is a wrapper for the [mkdocs](https://www.mkdocs.org/) suite that
is used to generate [this documentation](https://sorbus.xayax.net/). It
mostly makes sure that generated parts are not outdated and provides an
addition parameter for uploading.

### src/tools/testbuild.sh

This script checks out the current code into `../testbuild` and compiles
it there running `make all`, to make sure that what will be pushed is at
least buildable.

### src/tools/external-1kLEDsIsNoLimit.sh

This is a script that downloads my demo
[1k Is No Limit](https://xayax.net/1k_leds_is_no_limit/), assembles it,
and copies the binary file for deployment.

The binary is also checked in, so running this is usually not necessary.

### src/tools/external-cpm65.sh

This tries do download and compile the CP/M-65 operating system.

The binaries are also checked in, so running this is usually not necessary.

### src/tools/external-llvm-mos-sdk.sh

This tries to install the llvm-mos toolchain, required to build CP/M-65.
As CP/M-65 is also checked in, running this is usually not necessary.

### src/tools/external-taliforth2.sh

This downloads the latest version of
[TaliForth2](https://github.com/SamCoVT/TaliForth2). However this has
become a bit of a hit and miss, as some versions of Tali Forth 2 just
don't work on the Sorbus.

The binary is also checked in, so running this is usually not necessary.

## tl;dr

For setting up development on a new system, I typically run these
commands in the following order (no sudo required, as they run sudo
themselves):
```bash
make setup-apt-dev
src/tools/local-nstow.sh
src/tools/local-arm-none-eabi-toolchain.sh
src/tools/local-picotool.sh
```
