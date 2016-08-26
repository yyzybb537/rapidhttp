#!/bin/sh

set -e

git submodule update --init
mkdir build -p
cd build
cmake .. $@
cd ../
sudo rm /usr/local/include/rapidhttp -rf
sudo cp ./include/rapidhttp /usr/local/include/rapidhttp -rv
