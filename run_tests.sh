#!/bin/sh

# Copyright (c) 2016 Julien Palard.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
    LD_LIBRARY_PATH=. ./test /usr/bin/top
    exit
fi

$0 python
$0 c
