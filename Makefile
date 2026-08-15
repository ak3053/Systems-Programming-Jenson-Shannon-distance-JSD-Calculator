codeCC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -g
LDFLAGS = -lm

# CFLAGS += -fsanitize=address,undefined

TARGET  = compare
SRC     = compare.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)

	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
