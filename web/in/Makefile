.PHONY: all examples anim clean

ANIM_DIR=../../box/examples/smileys

all: anim examples
	r2w

anim:
	bash anim.bash

examples:
	(cd examples && make)

clean:
	rm -f bouncing.gif machine.gif

