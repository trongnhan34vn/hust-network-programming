CC = gcc
CFLAGS = -Wall
TARGET = resolver

all: $(TARGET)

$(TARGET): resolver.c
	$(CC) $(CFLAGS) -o $(TARGET) resolver.c

clean:
	rm -f $(TARGET) $(LOG_FILE)
