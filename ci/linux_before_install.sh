#!/usr/bin/env bash

sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo add-apt-repository ppa:ubuntu-sdk-team/ppa -y
# next 2 lines necessary until Ubuntu 14
wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | sudo apt-key add -
sudo add-apt-repository 'deb http://llvm.org/apt/precise/ llvm-toolchain-precise-3.5 main' -y
# done
sudo apt-get update -qq
