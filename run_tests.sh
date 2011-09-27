#!/bin/sh
make && make test && /lib/ld-linux.so.2 --library-path . ./test vttest
