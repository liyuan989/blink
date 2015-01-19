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

SERVER_SRC = ChatServer.cpp

CLIENT_SRC = ChatClient.cpp

HEADDIR = -I../../../src

LIBDIR = ../../../lib/debug/libblink.a

OUTPUTDIR = ../../../bin/asio/chat

SERVER_EXEC = $(OUTPUTDIR)/ChatServer

CLIENT_EXEC = $(OUTPUTDIR)/ChatClient

MKDIR = mkdir -p

RM = rm -f

all:
	$(MKDIR) $(OUTPUTDIR)
	$(CXX) $(CXXFLAGS) -o $(SERVER_EXEC) $(SERVER_SRC) $(LIBDIR) $(HEADDIR) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUTDIR)/ChatServerThreaded ChatServerThreaded.cpp $(LIBDIR) $(HEADDIR) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUTDIR)/ChatServerEfficient ChatServerEfficient.cpp $(LIBDIR) $(HEADDIR) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUTDIR)/ChatServerHighPerformance ChatServerHighPerformance.cpp $(LIBDIR) $(HEADDIR) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_EXEC) $(CLIENT_SRC) $(LIBDIR) $(HEADDIR) $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUTDIR)/ChatClientLoadTest ChatClientLoadTest.cpp $(LIBDIR) $(HEADDIR) $(LDFLAGS)

clean:
	$(RM) $(SERVER_EXEC) $(CLIENT_EXEC)

.PHONY: clean all
