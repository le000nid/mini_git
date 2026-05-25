CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude

TARGET = minigit

SRC = src/main.c src/minigit.c src/hash.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean