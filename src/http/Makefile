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

SRCS = *.cpp

HEAD = -I../

RM = rm -f

OBJS = $(SRCS:.cpp=.o)

rm_objs = $(RM) $(OBJS)

all:
	$(CXX) $(CXXFLAGS) $(SRCS) $(HEAD)

release:
	$(CXX) $(RELEASEFLAGS) $(CXXFLAGS) $(SRCS) $(HEAD)

clean:
	$(RM) $(OBJS)

.PHONY: all clean release

