#!/usr/bin/env bash

# Description: prerequisites for build project KanTV in command-line mode

# verified on Ubuntu 20.04, Ubuntu 24.04

sudo apt-get update
sudo apt-get install build-essential -y
sudo apt-get install cmake -y
sudo apt-get install curl -y
sudo apt-get install wget -y
sudo apt-get install git -y
sudo apt-get install vim -y
sudo apt-get install sed -y
sudo apt-get install openjdk-17-jdk -y

sudo apt-get install -y android-tools-adb autoconf \
        automake bc bison build-essential ccache cscope \
        expect flex libattr1-dev libcap-dev \
        libglib2.0-dev libncurses5-dev \
        libpixman-1-dev libssl-dev libtool make \
        mtools unzip xz-utils zlib1g-dev

echo "export PATH=/home/`whoami`/.local/bin:\$PATH" >> ~/.bashrc

#make hexagon-clang happy
if [ -f /lib/x86_64-linux-gnu/libtinfo.so.5 ]; then
    echo "libtinfo.so.5 already exist"
else
    echo "libtinfo.so.5 not exist"
    if [ -f /lib/x86_64-linux-gnu/libtinfo.so.6 ]; then
        echo "libtinfo.so.6 already exist"
        sudo  ln -sf /lib/x86_64-linux-gnu/libtinfo.so.6 /lib/x86_64-linux-gnu/libtinfo.so.5
    else
        echo "libtinfo.so.6 not exist"
    fi
fi
