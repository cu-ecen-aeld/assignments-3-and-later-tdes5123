# CROSS_COMPILE ?=

# CC ?= $(CROSS_COMPILE)gcc
# CFLAGS ?= -Wall -Wextra
# LDFLAGS ?=

# $(info ====================================)
# $(info = Building aesdsocket for target   =)
# $(info = CC = $(CC))
# $(info ====================================)

# all: aesdsocket
# debug: aesdsocket-d

# aesdsocket: aesdsocket.c
# 	$(CC) $(LDFLAGS) -o aesdsocket aesdsocket.c 

# aesdsocket-d: aesdsocket.c	
# 	$(CC) $(LDFLAGS) -g -O0 -o aesdsocket-d aesdsocket.c 

# clean:
# 	rm -f aesdsocket

CC ?= $(CC)
CFLAGS ?= $(CFLAGS)
LDFLAGS ?= $(LDFLAGS)

all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f aesdsocket *.o