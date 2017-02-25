PLATFORM=darwin

ifeq ($(PLATFORM),darwin)
	CC:=clang
	CFLAGS+=-I/usr/local/include/libusb-1.0
	LDFLAGS+=/usr/local/lib/libusb-1.0.a \
		-framework CoreFoundation \
		-framework IOKit
endif

CFLAGS+= -Wall -Werror

OBJECTS=powerman.o options.o lsusb.o power.o

all: powerman

powerman: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c powerman.h
	$(CC) -std=c99 $(CFLAGS) -c -o $@ $<

clean:
	rm -rf powerman $(OBJECTS)

.PHONY: clean all
