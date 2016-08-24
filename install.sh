#!/bin/sh

set -e

rm /usr/local/include/rapidhttp -rf
cp ./include/rapidhttp /usr/local/include/rapidhttp -rv
