PROGRAM = pinenote-usb-tablet
CC = cc
CFLAGS = -Wall -O2 $(shell pkg-config --cflags libusbgx)
LDFLAGS = $(shell pkg-config --libs libusbgx)

all: $(PROGRAM)

$(PROGRAM): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(PROGRAM)
