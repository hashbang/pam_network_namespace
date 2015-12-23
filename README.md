pam_network_namespace
=====================

A Linux PAM (Pluggable Authentication Module) that makes sessions run in their own network namespace.

A veth (Virtual Ethernet) bridge is created between the parent namespace and the new one.
This means each user has their own ethernet adapter.

Cleanup of veth devices is automatic once there are no processes left using the network namespace


## Building

Run `make`


## Dependencies

  - [libnl 3.x](http://www.infradead.org/~tgr/libnl/)

