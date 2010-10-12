.PHONY: all examples doc anim clean

ANIM_DIR=../../box/examples/smileys

all: anim examples doc
	r2w

anim:
	bash anim.bash

examples:
	mkdir -p ../out/examples
	(cd examples && make)

doc:
	(cd documentation && make)

clean:
	rm -f bouncing.gif machine.gif

