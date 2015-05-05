#!/usr/bin/env bash

sudo apt-get install -qq cmake
sudo apt-get install -qq libosmesa6-dev
sudo apt-get install -qq libqt5-dev qt5-qmake qtbase5-dev qt5-default qttools5-dev-tools

echo "$CXX"
if [ "$CXX" = "g++" ]; then sudo apt-get install -qq g++-4.8; fi
if [ "$CXX" = "g++" ]; then export CXX="g++-4.8" CC="gcc-4.8"; fi
