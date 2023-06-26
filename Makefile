CC = gcc
-std=gnu99 -Wall -Wextra -Werror -pedantic

build:
	$(CC) -o proj2 proj2.c -lpthread
clean:
	rm -f proj2