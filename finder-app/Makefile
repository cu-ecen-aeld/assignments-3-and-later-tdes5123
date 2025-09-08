CROSS_COMPILE ?=

CC := $(CROSS_COMPILE)gcc

$(info Using CC = $(CC))

writer: writer.o
	$(CC) -o writer writer.o

writer.o: writer.c
	$(CC) -c writer.c

clean:
	rm -f writer writer.o