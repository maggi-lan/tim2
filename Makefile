# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -g

# Target executable name
TARGET = tim2

# Object files needed for linking
OBJS = main.o rope.o editor.o display.o input.o

# Default target: build everything
all: $(TARGET)

# Link all object files into final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile main.c (depends on headers it includes)
main.o: main.c editor.h display.h input.h
	$(CC) $(CFLAGS) -c main.c

# Compile rope.c (depends on rope.h)
rope.o: rope.c rope.h
	$(CC) $(CFLAGS) -c rope.c

# Compile editor.c (depends on editor.h and rope.h)
editor.o: editor.c editor.h rope.h
	$(CC) $(CFLAGS) -c editor.c

# Compile display.c (depends on display.h and editor.h)
display.o: display.c display.h editor.h
	$(CC) $(CFLAGS) -c display.c

# Compile input.c (depends on input.h and editor.h)
input.o: input.c input.h editor.h
	$(CC) $(CFLAGS) -c input.c

# Clean up compiled files
clean:
	rm -f $(OBJS) $(TARGET)

# Mark targets that don't produce files
.PHONY: all clean
