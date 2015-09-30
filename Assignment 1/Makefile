BDIR = bin

_OBJS = sensor controller

OBJS = $(patsubst %,$(BDIR)/%,$(_OBJS))

CC = gcc

CFLAGS = -std=c99 -D_XOPEN_SOURCE

all: $(OBJS)

$(BDIR)/sensor: sensor.c message_queue.h
	@mkdir -p $(BDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BDIR)/controller: controller.c message_queue.h
	@mkdir -p $(BDIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BDIR)/*
	rmdir $(BDIR)
