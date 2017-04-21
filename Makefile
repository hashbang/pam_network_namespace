CFLAGS=-Wall -Wextra -D_GNU_SOURCE -O2 -fPIC $(shell pkg-config --cflags libnl-route-3.0)
LDFLAGS=-rdynamic $(shell pkg-config --libs libnl-route-3.0)
PREFIX=/usr/local
DESTDIR=

all: pam_network_namespace.so

pam_network_namespace.so: pam_network_namespace.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

test.bin: pam_network_namespace.c
	$(CC) $(CFLAGS) -DTEST -o $@ $^ $(LDFLAGS)

test: test.bin
	./test.bin echo "Test success!"

clean:
	rm -f pam_network_namespace.so pam_network_namespace.o test.bin

install: pam_network_namespace.so
	mkdir -p -- "$(DESTDIR)$(PREFIX)"/lib/security/
	install -- "$<" "$(DESTDIR)$(PREFIX)"/lib/security/

uninstall:
	rm -f -- "$(DESTDIR)$(PREFIX)"/lib/security/pam_network_namespace.so

.PHONY: all test clean install uninstall
