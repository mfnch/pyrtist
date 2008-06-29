.PHONY: all

EXAMPLES=*.example
all: index.html

index.html: thumbnails.dat

thumbnails.dat: $(EXAMPLES) create_example.py thumbnails.py
	rm -f thumbnails.dat
	for example in *.example; do \
	  python create_example.py $$example; \
	done
	cp *.png ../../out/examples/
	python thumbnails.py

