##
## Makefile for vt100
##
## Made by julien palard
## Login   <vt100@mandark.fr>
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
CFLAGS = -g3 -Wextra -Wstrict-prototypes -Wall -ansi -pedantic -I$(INCLUDE)
LIB = -lutil
RM = rm -f

$(NAME):	$(OBJ)
		$(CC) --shared $(OBJ) $(LIB) -o $(LINKERNAME)

test:	$(OBJ_TEST)
		$(CC) $(OBJ_TEST) -L . -l$(NAME) -o test

python_module:
		swig -python *.i
		python setup.py build_ext --inplace

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
