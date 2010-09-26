SUBDIRS = dylib

# Add files to the distribution generated with 'make dist'
EXTRA_DIST = bugs.bash common.bash scope.bash structure.bash destructors.bash \
             bytecode.bash \
	     test.py test_all.py ops.py structure.py subtypes.py procdef.py \
             scope.py bugs.py

check:
	(cd dylib && $(MAKE) buildtestdylib)
	python test_all.py
