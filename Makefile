#Makefile

RM = rm -rf

LIBDIR = lib

BINDIR = bin

EXECUTABLE = $(LIBDIR)/libblink.a

debug:
	cd src && $(MAKE)

release:
	cd src && $(MAKE) release

example:
	cd example && $(MAKE)

clean:
	$(RM) $(LIBDIR)

clean-example:
	$(RM) $(BINDIR)

clean-all: clean clean-example

all: debug release example

.PHONY: clean clean-all example release debug all
