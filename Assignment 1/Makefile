BDIR = bin

_BINS = sensor controller actuator cloud

BINS = $(patsubst %,$(BDIR)/%,$(_BINS))

CC = gcc

CFLAGS = -std=c99 -D_XOPEN_SOURCE

all: $(BINS)

$(BDIR)/sensor: sensor.c message_queue.h
	@mkdir -p $(BDIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BDIR)/controller: controller.c queue.c message_queue.h fifo.h queue.h
	@mkdir -p $(BDIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BDIR)/actuator: actuator.c message_queue.h
	@mkdir -p $(BDIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BDIR)/cloud: cloud.c message_queue.h fifo.h
	@mkdir -p $(BDIR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BDIR)/*
	rmdir $(BDIR)
