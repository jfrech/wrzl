INSTALL_TARGET = /usr/bin
MAN_INSTALL_TARGET = /usr/share/man


wrzl: wrzl.c
	cc -Wall -Wpedantic -Werror -O3 wrzl.c -o wrzl
	rm -f wrzl.o

.PHONY: install
install:
	make wrzl
	ln -f wrzl -T $(INSTALL_TARGET)/wrzl
	chown 0:wheel $(INSTALL_TARGET)/wrzl
	chmod 4750 $(INSTALL_TARGET)/wrzl
	ln -f wrzl.8 -T $(MAN_INSTALL_TARGET)/man8/wrzl.8
