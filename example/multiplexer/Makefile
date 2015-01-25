#Makefile
CXX = g++

CXXFLAGS = -g \
		   -Wconversion \
		   -Wall \
		   -Werror \
		   -Wextra \
		   -Wshadow \
		   -Wno-unused-parameter \
		   -Wold-style-cast \
		   -Woverloaded-virtual \
		   -Wpointer-arith \
		   -Wwrite-strings \
		   -Wreorder \
		   -march=native \
		   -rdynamic \

LDFLAGS = -pthread

HEADDIR = -I../../src

LIBDIR = ../../lib/debug/libblink.a

OUTPUTDIR = ../../bin/multiplexer

MKDIR = mkdir -p

RM = rm -f

all:
	$(MKDIR) $(OUTPUTDIR)
	$(CXX) $(CXXFLAGS) -o $(OUTPUTDIR)/MultiplexServer_simple MultiplexServer_simple.cpp $(LIBDIR) $(HEADDIR) $(LDFLAGS)

clean:
	$(RM) $(OUTPUTDIR)/Multi*

.PHONY: clean all
