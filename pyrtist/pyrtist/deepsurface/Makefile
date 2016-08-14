deep_buffer: image_buffer.h image_buffer.cc deep_buffer.h deep_buffer.cc \
  deep_surface.h deep_surface.cc
	g++ -Wall -std=c++11 -O2 \
	  image_buffer.cc deep_buffer.cc deep_surface.cc \
	  -lcairo -o deep_buffer

.PHONY: clean
clean:
	rm -f deep_buffer
