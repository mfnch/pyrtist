EXTRA_DIST = Makefile.dev mylib.c mylib.box test_mylib.box

buildtestdylib:
	$(MAKE) -f Makefile.dev LIBTOOL="$(LIBTOOL)" buildlib

clean:
	$(MAKE) -f Makefile.dev LIBTOOL="$(LIBTOOL)" clean
