#!/usr/bin/env bash

# Copyright (c) 2021- KanTV Authors

# Description: prerequisites for build project
#
sudo apt-get update
sudo apt-get install build-essential -y
sudo apt-get install cmake -y
sudo apt-get install curl -y
sudo apt-get install wget -y
sudo apt-get install python -y
sudo apt-get install tcl expect -y
sudo apt-get install nginx -y
sudo apt-get install git -y
sudo apt-get install vim -y
sudo apt-get install spawn-fcgi -y
sudo apt-get install u-boot-tools -y
sudo apt-get install ffmpeg -y
sudo apt-get install openssh-client -y
sudo apt-get install nasm -y
sudo apt-get install yasm -y
sudo apt-get install openjdk-17-jdk -y

sudo dpkg --add-architecture i386
sudo apt-get install lib32z1 -y

sudo apt-get install -y android-tools-adb android-tools-fastboot autoconf \
        automake bc bison build-essential ccache cscope curl device-tree-compiler \
        expect flex ftp-upload gdisk acpica-tools libattr1-dev libcap-dev \
        libfdt-dev libftdi-dev libglib2.0-dev libhidapi-dev libncurses5-dev \
        libpixman-1-dev libssl-dev libtool make \
        mtools netcat python-crypto python3-crypto python-pyelftools \
        python3-pycryptodome python3-pyelftools python3-serial \
        rsync unzip uuid-dev xdg-utils xterm xz-utils zlib1g-dev

sudo apt-get install python3-pip -y
sudo apt-get install indent -y
pip3 install meson ninja

echo "export PATH=/home/`whoami`/.local/bin:\$PATH" >> ~/.bashrc
