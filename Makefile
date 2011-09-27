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

SRC = terminal.c terminal_vt100.c vt100_headless.c
SRC_TEST = test.c
OBJ = $(SRC:.c=.o)
OBJ_TEST = $(SRC_TEST:.c=.o)
CC = gcc
INCLUDE = .
DEFINE = _GNU_SOURCE
CFLAGS = -g3 -Wextra -Wstrict-prototypes -Wall -ansi -pedantic -I$(INCLUDE)
LIB = -lutil
RM = rm -f

$(NAME):	$(OBJ)
		$(CC) --shared $(OBJ) $(LIB) -o $(LINKERNAME)

test:	$(OBJ_TEST)
		$(CC) $(OBJ_TEST) -L . -l$(NAME) -o test

all:
		@make $(NAME)

.c.o:
		$(CC) -D $(DEFINE) -c $(CFLAGS) $< -o $(<:.c=.o)

clean:
		$(RM) $(LINKERNAME) test *~ \#*\# *.o *core

re:		clean all

check-syntax:
		gcc -Wall -Wextra -ansi -pedantic -o /dev/null -S ${CHK_SOURCES}
