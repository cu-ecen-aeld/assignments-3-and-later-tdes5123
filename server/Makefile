CROSS_COMPILE ?=

CC ?= $(CROSS_COMPILE)gcc

$(info ====================================)
$(info = Building aesdsocket for target   =)
$(info = CC = $(CC))
$(info ====================================)

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) -o aesdsocket aesdsocket.c

clean:
	rm -f aesdsocket