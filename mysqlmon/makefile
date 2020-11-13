
# makefile for mysqlmon

CC = gcc
CFLAGS = -lncurses -Ofast -Wall -Wextra -Wuninitialized -Wunused -Werror -std=gnu99 -s
NAME = mysqlmon

MYSQLCFLAGS = $(shell mysql_config --cflags)
MYSQLLIBS = $(shell mysql_config --libs)

$(NAME):
	$(CC) $(NAME).c -o $(NAME) $(CFLAGS) $(MYSQLCFLAGS) $(MYSQLLIBS)

install:
	sudo cp $(NAME) /usr/local/bin/$(NAME)
	@echo "Attempted to copy $(NAME) to /usr/local/bin"

deps:
	sudo apt install libmysqlclient-dev
