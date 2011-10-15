##
## Makefile for lw_vt100 examples
##
## Made by julien palard <vt100@mandark.fr>
##

NAME = parser

SRC = parser.c ../src/lw_terminal_parser.c
OBJ = $(SRC:.c=.o)
CC = gcc
INCLUDE = ../src
DEFINE = _GNU_SOURCE
CFLAGS = -g3 -Wextra -Wstrict-prototypes -Wall -ansi -pedantic -I$(INCLUDE)
LIB = -lutil
RM = rm -f

$(NAME):	$(OBJ)
		$(CC) $(OBJ) $(LIB) -o $(NAME)

all:
		@make $(NAME)

.c.o:
		$(CC) -D $(DEFINE) -c $(CFLAGS) $< -o $(<:.c=.o)

clean:
		$(RM) $(NAME) *~ \#*\# *.o *core

re:		clean all

check-syntax:
		gcc -Isrc -Wall -Wextra -ansi -pedantic -o /dev/null -S ${CHK_SOURCES}
