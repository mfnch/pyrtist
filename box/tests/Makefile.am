SUBDIRS = dylib

# Add files to the distribution generated with 'make dist'
EXTRA_DIST = bugs.bash common.bash scope.bash structure.bash destructors.bash \
             bytecode.bash \
	     test.py test_all.py operator_prec.py procdef.py scope.py

check:
	(cd dylib && $(MAKE) buildtestdylib)
	python test_all.py
