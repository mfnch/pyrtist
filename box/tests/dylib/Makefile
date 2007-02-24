all: run

libmylib.so: mylib.c
	gcc -I../../src -fPIC -shared -o libmylib.so mylib.c

run: libmylib.so
	../../src/box -v -L ./ -l mylib -o test_mylib.bvm test_mylib.box

check: run

.PHONY: all run check
