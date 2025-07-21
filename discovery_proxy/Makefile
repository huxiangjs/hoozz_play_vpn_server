##################### Makefile ###################

.PHONY: all clean

CC := gcc
CFLAGS := -O2 -Wall
OBJS := main.o
TAG := discover_proxy

all: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(TAG)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@ -lpthread

clean:
	rm -f $(OBJS) $(TAG)

