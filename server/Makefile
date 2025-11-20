CROSS_COMPILE ?=

CC ?= $(CROSS_COMPILE)gcc

$(info ====================================)
$(info = Building aesdsocket for target   =)
$(info = CC = $(CC))
$(info ====================================)

all: aesdsocket
debug: aesdsocket-d

aesdsocket: aesdsocket.c
	$(CC) -o aesdsocket aesdsocket.c

aesdsocket-d: aesdsocket.c	
	$(CC) -g -O0 -o aesdsocket-d aesdsocket.c

clean:
	rm -f aesdsocket