all: pam_network_namespace.so

CFLAGS=-Wall -Wextra -Werror -D_GNU_SOURCE -O2 -fPIC

pam_network_namespace.so: pam_network_namespace.o
	$(CC) -shared -o $@ $< -rdynamic

clean:
	rm -f pam_network_namespace.so pam_network_namespace.o

.PHONY: clean
