##
## Makefile for vt100
##
## Made by julien palard
## Login   <vt100@mandark.fr>
##
## Copyright (c) 2016 Julien Palard.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
## 1. Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer.
## 2. Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in the
##    documentation and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
## IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
## OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
## INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
## NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
## THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

NAME = vt100
VERSION = 0
MINOR = 0
RELEASE = 0

LINKERNAME = lib$(NAME).so
SONAME = $(LINKERNAME).$(VERSION)
REALNAME = $(SONAME).$(MINOR).$(RELEASE)

SRC = src/lw_terminal_parser.c src/lw_terminal_vt100.c src/hl_vt100.c
SRC_TEST = src/test.c
OBJ = $(SRC:.c=.o)
OBJ_TEST = $(SRC_TEST:.c=.o)
CC = gcc
INCLUDE = src
DEFINE = _GNU_SOURCE
CFLAGS = -DNDEBUG -g3 -Wextra -Wstrict-prototypes -Wall -ansi -pedantic -fPIC -I$(INCLUDE)
LIB = -lutil
RM = rm -f

$(NAME):	$(OBJ)
		$(CC) --shared $(OBJ) $(LIB) -o $(LINKERNAME)

test:	$(OBJ_TEST)
		$(CC) $(OBJ_TEST) -L . -l$(NAME) -o test

python_module:
		swig -python -threads *.i

all:
		@make $(NAME)

.c.o:
		$(CC) -D $(DEFINE) -c $(CFLAGS) $< -o $(<:.c=.o)

clean_python_module:
		$(RM) *.pyc *.so hl_vt100_wrap.c hl_vt100.py
		$(RM) -r build

clean:	clean_python_module
		$(RM) $(LINKERNAME) test src/*~ *~ src/\#*\# src/*.o \#*\# *.o *core

re:		clean all

check-syntax:
		gcc -Isrc -Wall -Wextra -ansi -pedantic -o /dev/null -S ${CHK_SOURCES}
