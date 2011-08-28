##
## Makefile for vt100
##
## Made by julien palard
## Login   <vt100@mandark.fr>
##

NAME = vt100
SRC = vt100.c
OBJ = $(SRC:.c=.o)
CC = gcc
INCLUDE = .
DEFINE = _GNU_SOURCE
CFLAGS = -O3 -Wextra -Wall -ansi -pedantic -I$(INCLUDE)
RM = rm -f

$(NAME):	$(OBJ)
		$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LIB)

all:
		@make $(NAME)

.c.o:
		$(CC) -D $(DEFINE) -c $(CFLAGS) $< -o $(<:.c=.o)

clean:
		$(RM) $(NAME) *~ #*# *.o *.core

re:		clean all
