all: run_mylib

libmylib.so: mylib.c
	gcc -I../.. -fPIC -shared -o libmylib.so mylib.c

run_mylib: libmylib.so
	../../box/box -v -L ./ -l mylib -o test_mylib.bvm test_mylib.box

libline.so: line.c
	gcc -I../../src -fPIC -shared -o $@ $<

run_line: libline.so
	../../box/box -v -L ./ -l line -o test_line.bvm test_line.box

check: run_mylib

.PHONY: all run_mylib run_line check
