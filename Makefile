CC = gcc
CFLAGS = -g -Wall -Werror

SRC = main.c

HEADERS =

OBJ = $(SRC:.c=.o)

all: main
	clear && ./main
main: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c $(HEADERS) clean
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main $(OBJ)