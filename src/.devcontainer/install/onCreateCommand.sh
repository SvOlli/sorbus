#!/bin/bash
echo "#######################################################"
echo "### Install additional software                     ###"
echo "#######################################################"

sudo apt update
sudo apt install -y --no-install-recommends \
  make \
  cmake \
  git \
  wget \
  udev \
  python3 \
  python3-venv \
  python3-pip \
  cc65 \
  cpmtools \
  gcc-arm-none-eabi \
  libnewlib-arm-none-eabi \
  libstdc++-arm-none-eabi-dev \
  libstdc++-arm-none-eabi-newlib \
  gdb-multiarch \
  binutils-multiarch \
  openocd \
  minicom


# Enable profile for zsh
echo "[[ -e $PROFILE_FILE ]] && emulate sh -c 'source $PROFILE_FILE'" >> $HOME/.zshrc


