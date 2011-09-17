##
## Makefile for vt100
##
## Made by julien palard
## Login   <vt100@mandark.fr>
##

NAME = vt100
SRC = term.c test.c
OBJ = $(SRC:.c=.o)
CC = gcc
INCLUDE = .
DEFINE = _GNU_SOURCE
CFLAGS = -g3 -Wextra -Wstrict-prototypes -Wall -ansi -pedantic -I$(INCLUDE)
LIB = -lutil
RM = rm -f

$(NAME):	$(OBJ)
		$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LIB)

all:
		@make $(NAME)

.c.o:
		$(CC) -D $(DEFINE) -c $(CFLAGS) $< -o $(<:.c=.o)

clean:
		$(RM) $(NAME) *~ \#*\# *.o *.core

re:		clean all

check-syntax:
		gcc -o /dev/null -S ${CHK_SOURCES}
