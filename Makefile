CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = auth_server
SOURCE = auth_server.c

# Default target
all: $(TARGET)

# Compile the server
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) -lcrypt

# Run the server
run: $(TARGET)
	./$(TARGET)

# Clean build files
clean:
	rm -f $(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install build-essential

.PHONY: all run clean install-deps
