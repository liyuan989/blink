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

SERVER_SRC = main.cpp EchoServer.cpp

HEADDIR = -I../../src

LIBDIR = ../../lib/debug/libblink.a

OUTPUTDIR = ../../bin/idleconnection

SERVER_EXEC = $(OUTPUTDIR)/server


MKDIR = mkdir -p

RM = rm -f

all:
	$(MKDIR) $(OUTPUTDIR)
	$(CXX) $(CXXFLAGS) -o $(SERVER_EXEC) $(SERVER_SRC) $(LIBDIR) $(HEADDIR) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUTDIR)/server_sorted EchoServer_sorted.cpp $(LIBDIR) $(HEADDIR) $(LDFLAGS)

clean:
	$(RM) $(SERVER_EXEC) $(CLIENT_EXEC)

.PHONY: clean all
