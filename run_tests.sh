#!/bin/sh
if [ "$1" = python ]
then
    if ! which swig > /dev/null
    then
        echo "You should install swig !"
        exit 1
    fi
    make python_module
    if [ $? != 0 ]
    then
        echo "Failed to build python module"
        exit 1
    fi
    cat <<EOF | python
import hl_vt100
import time
import sys

print "Starting python test..."
vt100 = hl_vt100.vt100_headless()
vt100.fork('/usr/bin/top', ['/usr/bin/top', '-n', '1'])
vt100.main_loop()
[sys.stdout.write(line + "\n") for line in vt100.getlines()]
EOF
    exit
fi

make clean

if [ "$1" = c ]
then
    make && make test
    if [ -x /lib/ld-linux-*.so.2 ]
    then
        echo Using /lib/ld-linux-*.so.2 to run test
        /lib/ld-linux-*.so.2 --library-path . ./test /usr/bin/top
    else
        echo Using /lib/ld-linux.so.2 to run test
        /lib/ld-linux.so.2 --library-path . ./test /usr/bin/top
    fi
    exit
fi

$0 python
$0 c