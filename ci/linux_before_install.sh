#!/usr/bin/env bash

sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo add-apt-repository ppa:ubuntu-sdk-team/ppa -y
sudo apt-get update -qq
sudo apt-get install -qq qt5-qmake qtbase5-dev
