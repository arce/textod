PROG     := textod
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -ffp-contract=off
PREFIX   ?= /usr/local
DATADIR  ?= $(PREFIX)/share/$(PROG)
DEFS     :=

HEADERS  := $(shell find src -name '*.hpp')

.PHONY: all test check install uninstall clean help

all: build/$(PROG)

help:
	@echo "make            builds build/$(PROG)"
	@echo "make check      renders examples/*.dg and compares them with the .svg next to them"
	@echo "make install    installs into $(PREFIX)/bin and the styles and themes into $(DATADIR) (PREFIX=... to change it)"
	@echo "make clean      removes build/"

build:
	mkdir -p $@

build/$(PROG): src/$(PROG).cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $(DEFS) -DPROGRAM=\"$(PROG)\" -Isrc -o $@ $<

test: check

check: build/$(PROG)
	@rm -rf build/out; mkdir -p build/out; fail=0; \
	for f in examples/*.dg; do \
		build/$(PROG) $$f -o build/out/$$(basename $${f%.dg}).svg || { echo "FAIL  $$f"; fail=1; }; \
	done; \
	for f in build/out/*.svg; do n=$$(basename $$f); \
		if cmp -s $$f examples/$$n; then echo "ok    $$n"; else echo "DIFF  $$n"; fail=1; fi; \
	done; \
	[ $$fail = 0 ] && echo "examples ok"; exit $$fail

install: build/$(PROG)
	install -d $(PREFIX)/bin
	install -m 755 build/$(PROG) $(PREFIX)/bin/$(PROG)
	install -d $(DATADIR)/styles
	install -m 644 lib/styles/*.dg $(DATADIR)/styles
	install -d $(DATADIR)/themes
	install -m 644 lib/themes/*.css $(DATADIR)/themes

uninstall:
	rm -f $(PREFIX)/bin/$(PROG)
	rm -rf $(DATADIR)

clean:
	rm -rf build
