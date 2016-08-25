#!/bin/sh

set -e

git submodule update --init
./extract_http_parser.sh .
sudo rm /usr/local/include/rapidhttp -rf
sudo cp ./include/rapidhttp /usr/local/include/rapidhttp -rv
