PROG     := textod
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -ffp-contract=off
PREFIX   ?= /usr/local
DEFS     := 

HEADERS  := $(shell find src -name '*.hpp')
LIBS     := $(shell find lib -name '*.dg' -o -name '*.css')
TESTS    := $(patsubst tests/%.cpp,build/%,$(wildcard tests/*_test.cpp))

.PHONY: all test check install uninstall clean help

all: build/$(PROG)

help:
	@echo "make            builds build/$(PROG)"
	@echo "make test       unit tests and the examples"
	@echo "make check      only the examples (examples/*.dg against their expected output)"
	@echo "make install    installs into $(PREFIX)/bin (PREFIX=... to change it)"
	@echo "make clean      removes build/"

build:
	mkdir -p $@

# the libraries of this program are embedded in the executable
build/embedded_lib.hpp: $(LIBS) tools/embed.py | build
	python3 tools/embed.py lib $@

build/$(PROG): src/$(PROG).cpp $(HEADERS) build/embedded_lib.hpp | build
	$(CXX) $(CXXFLAGS) $(DEFS) -DPROGRAM=\"$(PROG)\" -Ibuild -Isrc -o $@ $<

build/%_test: tests/%_test.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $(DEFS) -Isrc -o $@ $<

test: $(TESTS) check
	@for t in $(TESTS); do echo "== $$t"; ./$$t || exit 1; done

check: build/$(PROG)
	@for f in examples/*.dg; do \
	  build/$(PROG) $$f -o build/example.svg || { echo "FAIL $$f"; exit 1; }; \
	  cmp -s build/example.svg $${f%.dg}.svg || { echo "DIFFERENT $$f"; exit 1; }; \
	done; echo "examples ok"

install: build/$(PROG)
	install -d $(PREFIX)/bin
	install -m 755 build/$(PROG) $(PREFIX)/bin/$(PROG)

uninstall:
	rm -f $(PREFIX)/bin/$(PROG)

clean:
	rm -rf build
