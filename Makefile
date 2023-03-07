TARGET	= pa0
CFLAGS	= -g -c -D_POSIX_C_SOURCE -D_XOPEN_SOURCE=700
CFLAGS += -std=c99 -Werror
CFLAGS += # Add your own cflags here if necessary
LDFLAGS	=

all: pa0

pa0: pa0.o stack.o
	gcc $(LDFLAGS) $^ -o $@

%.o: %.c
	gcc $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET) *.o *.dSYM
