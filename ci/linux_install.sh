#!/usr/bin/env bash

sudo apt-get install -qq cmake
sudo apt-get install -qq libosmesa6-dev
sudo apt-get install -qq libqt4-dev qt4-qmake

if [ "$CXX" = "g++" ]; then
  sudo apt-get install -qq g++-4.8
  export CXX="g++-4.8" CC="gcc-4.8"
  echo "$CXX"
fi

if [ "$CXX" = "clang++" ]; then
  sudo apt-get install -qq llvm-3.5 llvm-3.5-dev clang-3.5 libstdc++-4.9-dev
  sudo apt-get remove llvm clang
  export CXX="clang++-3.5" CC="clang-3.5"
  $CXX --version
fi
