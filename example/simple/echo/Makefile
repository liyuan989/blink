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

SERVER_SRC = server.cpp EchoServer.cpp

CLIENT_SRC = client.cpp EchoClient.cpp

HEADDIR = -I../../../src

LIBDIR = ../../../lib/debug/libblink.a

OUTPUTDIR = ../../../bin/simple/echo

SERVER_EXEC = $(OUTPUTDIR)/server

CLIENT_EXEC = $(OUTPUTDIR)/client

MKDIR = mkdir -p

RM = rm -f

all:
	$(MKDIR) $(OUTPUTDIR)
	$(CXX) $(CXXFLAGS) -o $(SERVER_EXEC) $(SERVER_SRC) $(LIBDIR) $(HEADDIR) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_EXEC) $(CLIENT_SRC) $(LIBDIR) $(HEADDIR) $(LDFLAGS)

clean:
	$(RM) $(SERVER_EXEC) $(CLIENT_EXEC)

.PHONY: clean all
