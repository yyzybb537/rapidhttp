#!/bin/sh

dest=$1/include/rapidhttp/layer.hpp

echo "#pragma once" > $dest
cat $1/third_party/picohttpparser/picohttpparser.h >> $dest
sed -i 's/extern\ "C"/namespace rapidhttp/g' $dest

last_include=`grep "^\#include" $1/third_party/picohttpparser/picohttpparser.c -n | tail -1 | cut -d: -f1`
tail_start=`expr $last_include + 1`

head -$last_include $1/third_party/picohttpparser/picohttpparser.c >> $dest

echo "namespace rapidhttp {" >> $dest
tail -n +$tail_start $1/third_party/picohttpparser/picohttpparser.c >> $dest
echo "} //namespace rapidhttp" >> $dest

# add inline key-word
sed -i 's/^int phr_parse_request(.*/inline &/g' $dest
sed -i 's/^int phr_parse_response(.*/inline &/g' $dest
sed -i 's/^int phr_parse_headers(.*/inline &/g' $dest
sed -i 's/^ssize_t phr_decode_chunked(.*/inline &/g' $dest
sed -i 's/^int phr_decode_chunked_is_in_data(.*/inline &/g' $dest

sed -i 's/^static .*(.*/inline &/g' $dest
sed -i 's/^\#include "picohttpparser.h"//g' $dest

echo "create pico $dest"
