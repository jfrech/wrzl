INSTALL_TARGET = /usr/bin


wrzl: wrzl.c
	cc -Wall -Wpedantic -Werror -O3 wrzl.c -o wrzl
	rm -f wrzl.o

.PHONY: install
install:
	make wrzl
	mv wrzl $(INSTALL_TARGET)/wrzl
	chown 0:wheel $(INSTALL_TARGET)/wrzl
	chmod 4750 $(INSTALL_TARGET)/wrzl
