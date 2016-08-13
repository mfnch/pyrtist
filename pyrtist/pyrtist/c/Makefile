depth_buffer: image_buffer.h image_buffer.cc depth_buffer.h depth_buffer.cc \
  depth_surface.h depth_surface.cc
	g++ -Wall -std=c++11 -O2 \
	  image_buffer.cc depth_buffer.cc depth_surface.cc \
	  -lcairo -o depth_buffer

.PHONY: clean
clean:
	rm -f depth_buffer
