
# makefile for mysqllockmon

CC = gcc

NAME = mysqllockmon

INCLUDE = -I../mysql_include/

CFLAGS = -lncurses -Ofast -Wall -Wextra -Wuninitialized -Wunused -Wformat=2 -Wunused-parameter -Wshadow -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs -Wformat-security -std=gnu99 -flto -s

MYSQLCFLAGS = $(shell mysql_config --cflags)

MYSQLLIBS = $(shell mysql_config --libs)


$(NAME):
	$(CC) $(NAME).c -o $(NAME) $(INCLUDE) $(CFLAGS) $(MYSQLCFLAGS) $(MYSQLLIBS)

install:
	sudo cp $(NAME) /usr/local/bin/$(NAME)
	@echo "Attempted to copy $(NAME) to /usr/local/bin"

deps:
	sudo apt install libmysqlclient-dev libncurses5-dev
