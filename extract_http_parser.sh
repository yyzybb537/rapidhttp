#!/bin/sh

dest=$1/include/rapidhttp/http_parser.hpp

echo "#pragma once" > $dest
cat $1/third_party/http-parser/http_parser.h >> $dest
sed -i 's/extern\ "C"/namespace rapidhttp/g' $dest
echo "namespace rapidhttp {" >> $dest
cat $1/third_party/http-parser/http_parser.c | grep "^#include" -v >> $dest
echo "} //namespace rapidhttp" >> $dest

# add inline key-word
sed -i 's/^unsigned long http_parser_version(void);/inline &/g' $dest
sed -i 's/^void http_parser_init(.*);/inline &/g' $dest
sed -i 's/^void http_parser_settings_init(.*);/inline &/g' $dest
sed -i 's/^size_t http_parser_execute\s*(.*/inline &/g' $dest
sed -i 's/^int http_should_keep_alive(.*);/inline &/g' $dest

sed -i 's/^const char \*http_method_str(.*);/inline &/g' $dest
sed -i 's/^const char \*http_errno_name(.*);/inline &/g' $dest
sed -i 's/^const char \*http_errno_description(.*);/inline &/g' $dest
sed -i 's/^void http_parser_url_init(.*);/inline &/g' $dest

sed -i 's/^int http_parser_parse_url(.*/inline &/g' $dest
sed -i 's/^void http_parser_pause(.*);/inline &/g' $dest
sed -i 's/^int http_body_is_final(.*);/inline &/g' $dest

sed -i 's/^unsigned long$/inline &/g' $dest
sed -i 's/^static enum state$/inline &/g' $dest
sed -i 's/^static enum http_host_state$/inline &/g' $dest
sed -i 's/^int$/inline &/g' $dest
sed -i 's/^static int$/inline &/g' $dest
sed -i 's/^void$/inline &/g' $dest
sed -i 's/^const char \*$/inline &/g' $dest

echo "create $dest"
