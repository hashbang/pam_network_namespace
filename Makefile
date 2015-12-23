CFLAGS=-Wall -Wextra -Werror -D_GNU_SOURCE -O2 -fPIC `pkg-config --cflags libnl-route-3.0`

all: pam_network_namespace.so

pam_network_namespace.so: pam_network_namespace.o
	$(CC) -shared -o $@ $< -rdynamic `pkg-config --libs libnl-route-3.0`

clean:
	rm -f pam_network_namespace.so pam_network_namespace.o

.PHONY: all clean
