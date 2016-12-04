# This makefile is useful for building the library locally so that it can be
# used with local_launch.py for running pyrtist without having to install it.

OUT_LIB=../deepsurface.so
CXX = g++
CAIRO_LIBS = `pkg-config cairo --libs`
CAIRO_CFLAGS = `pkg-config cairo --cflags`
CXXFLAGS := -std=c++11 -g -O2 -Wall -pthread -fPIC -fwrapv \
  -fno-strict-aliasing -fstack-protector-strong -Wformat \
  -Werror=format-security -DNDEBUG -D_FORTIFY_SOURCE=2 \
  -I/usr/include/python2.7
SOURCES := depth_buffer.cc deep_surface.cc image_buffer.cc py_depth_buffer.cc \
  py_image_buffer.cc mesh.cc obj_parser.cc py_init.cc
OBJS := $(SOURCES:.cc=.o)

.PHONY: clean

$(OUT_LIB): $(OBJS) $(HEADERS)
	$(CXX) -shared -Wl,-O1 -Wl,-Bsymbolic-functions -Wl,-z,relro \
	  $(OBJS) $(CAIRO_LIBS) -o $(OUT_LIB)

-include $(SOURCES:.cc=.d)

%.o: %.cc
	$(CXX)  -c $(CXXFLAGS) $(CAIRO_CFLAGS) $*.cc -o $*.o
	$(CXX) -MM $(CXXFLAGS) $(CAIRO_CFLAGS) $*.cc > $*.d

clean:
	rm -f *.o *.d $(OUT_LIB)
