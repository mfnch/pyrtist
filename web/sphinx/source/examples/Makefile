BOX_EXAMPLES="snake,cycloid,wheatstone,multivibrator,yin-yang,curved_arrow,box-logo,translucency,gradient,fractree,pythagoras,text,cairo_set_line_cap,cairo_gradient"
BOXER_EXAMPLES="fem2d,fem,gmr_bands,gmr,es_types,constriction,fivelayers,llg,hd,orchester"

.PHONY: all clean build-examples

all: build-examples gallery.rst

build-examples: box/Makefile boxer/Makefile
	(cd box && python ../copy_examples.py *.example)
	(cd box && make)
	(cd boxer && make)

box/Makefile: gen_makefile.sh
	bash gen_makefile.sh box > $@

boxer/Makefile: gen_makefile.sh
	bash gen_makefile.sh boxer > $@

gallery.rst: gallery.rst.in
	python thumbnails.py box $(BOX_EXAMPLES) boxer $(BOXER_EXAMPLES)

clean:
	(cd box && make clean)
	(cd boxer && make clean)

