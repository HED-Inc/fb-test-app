
CC=$(CROSS_COMPILE)gcc
CFLAGS=-I/home/valkeine/work/linux/usr/include -O2 -Wall

all: pan readback upd perf rect

clean:
	rm -f pan readback upd perf rect
