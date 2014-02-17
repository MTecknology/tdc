export PREFIX ?= /usr/local

all: tdc tdc.1

debug:
	$(CC) -O2 -Wall -g3 -ggdb tdc.c -o tdc $(shell pkg-config x11 xft --libs --cflags)

install: tdc tdc.1
	install -p -m 644 -D tdc.1.gz ${DESTDIR}${PREFIX}/share/man/man1/tdc.1.gz
	install -p -m 755 -D tdc ${DESTDIR}${PREFIX}/bin/tdc

uninstall:
	-rm -f ${DESTDIR}${PREFIX}/share/man/man1/tdc.1.gz
	-rm -f ${DESTDIR}${PREFIX}/bin/tdc

clean:
	-rm -f tdc.1.gz
	-rm -f tdc

tdc:
	$(CC) -O2 tdc.c -o tdc $(shell pkg-config x11 xft --libs --cflags)

tdc.1:
	gzip -c docs/tdc.1 > tdc.1.gz
