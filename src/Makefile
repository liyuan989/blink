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
           -c

RELEASEFLAGS = -DNDEBUG -O2

AR = ar

ARFLAGS = crv

RM = rm -f

MKDIR = mkdir -p

SRCS = *.cpp

OBJS = $(SRCS:.cpp=.o) http/*.o

LIBDIR_DEBUG = ../lib/debug

LIBDIR_RELEASE = ../lib/release

EXECUTABLE_DEBUG = $(LIBDIR_DEBUG)/libblink.a

EXECUTABLE_RELEASE = $(LIBDIR_RELEASE)/libblink.a

ar_lib_debug = $(AR) $(ARFLAGS) $(EXECUTABLE_DEBUG) $(OBJS)

ar_lib_release = $(AR) $(ARFLAGS) $(EXECUTABLE_RELEASE) $(OBJS)

rm_objs = $(RM) $(OBJS)

all:
	$(MKDIR) $(LIBDIR_DEBUG)
	$(CXX) $(CXXFLAGS) $(SRCS)
	cd http && $(MAKE)
	$(ar_lib_debug)
	$(rm_objs)

release:
	$(MKDIR) $(LIBDIR_RELEASE)
	$(CXX) $(RELEASEFLAGS) $(CXXFLAGS) $(SRCS)
	cd http && $(MAKE) release
	$(ar_lib_release)
	$(rm_objs)

clean:
	$(RM) $(EXECUTABLE_DEBUG) $(EXECUTABLE_RELEASE)

.PHONY: all clean release

