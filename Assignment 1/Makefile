BDIR = bin

_OBJS = sensor

OBJS = $(patsubst %,$(BDIR)/%,$(_OBJS))

CC = gcc

all: $(OBJS)

$(BDIR)/sensor: sensor.c
	@mkdir -p $(BDIR)
	$(CC) -o $@ $<

clean:
	rm -f $(BDIR)/*
	rmdir $(BDIR)
