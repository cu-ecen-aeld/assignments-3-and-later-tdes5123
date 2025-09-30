CROSS_COMPILE ?=

CC := $(CROSS_COMPILE)gcc

$(info Using CC = $(CC))

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) -o aesdsocket aesdsocket.c

clean:
	rm -f aesdsocket