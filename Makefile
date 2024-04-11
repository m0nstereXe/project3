CC = gcc
CFLAGS = -g -c -m32
AR = ar -rc
RANLIB = ranlib

# Source files
SRCS = my_vm.c test.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executables
EXECUTABLES = my_vm.a test

# Default target
all: $(EXECUTABLES)

# Static library target
my_vm.a: my_vm.o
	$(AR) $@ $^
	$(RANLIB) $@

# Test executable target
test: test.o my_vm.o
	$(CC) -o $@ $^ -m32

# Compile rule for all .c files
%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

# Clean target
clean:
	rm -rf *.o *.a test
