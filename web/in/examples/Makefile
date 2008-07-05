.PHONY: all force

EXAMPLES=*.example
all: index.txt

index.txt: thumbnails.dat index.txt.in
	python thumbnails.py

thumbnails.dat: $(EXAMPLES) create_example.py thumbnails.py
	make force

force:
	rm -f thumbnails.dat
	for example in *.example; do \
	  python create_example.py $$example; \
	done
	cp *.png ../../out/examples/
