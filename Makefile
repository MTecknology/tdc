export PREFIX ?= /usr/local

PKG_CONFIG ?= pkg-config
CPPFLAGS += $(shell $(PKG_CONFIG) x11 xft --cflags)
LIBS += $(shell $(PKG_CONFIG) x11 xft --libs)

all: tdc tdc.1

install: tdc tdc.1
	install -p -m 644 -D tdc.1.gz ${DESTDIR}${PREFIX}/share/man/man1/tdc.1.gz
	install -p -m 755 -D tdc ${DESTDIR}${PREFIX}/bin/tdc

uninstall:
	rm -f ${DESTDIR}${PREFIX}/share/man/man1/tdc.1.gz
	rm -f ${DESTDIR}${PREFIX}/bin/tdc

clean:
	rm -f tdc.1.gz
	rm -f tdc

tdc: tdc.c
	$(CC) -O2 -g $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) tdc.c -o tdc $(LIBS)

tdc.1: docs/tdc.1
	gzip -c docs/tdc.1 > tdc.1.gz

.PHONY: all clean install uninstall
