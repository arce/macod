PROG     := macod
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -ffp-contract=off
PREFIX   ?= /usr/local
DATADIR  ?= $(PREFIX)/share/$(PROG)

HEADERS  := $(shell find src -name '*.hpp')
TESTS    := $(patsubst tests/%.cpp,build/%,$(wildcard tests/*_test.cpp))

.PHONY: all test check install uninstall clean help

all: build/$(PROG)

help:
	@echo "make            builds build/$(PROG)"
	@echo "make test       unit tests and the examples"
	@echo "make check      only the examples (examples/*.dg against their expected output)"
	@echo "make install    installs into $(PREFIX)/bin and the libraries into $(DATADIR) (PREFIX=... to change it)"
	@echo "make clean      removes build/"

build:
	mkdir -p $@

# the libraries (lib/) are files that macod reads when it runs: nothing is embedded in the executable
build/$(PROG): src/$(PROG).cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) -DPROGRAM=\"$(PROG)\" -Isrc -o $@ $<

build/%_test: tests/%_test.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<

test: $(TESTS) check
	@for t in $(TESTS); do echo "== $$t"; ./$$t || exit 1; done

check: build/$(PROG)
	@for f in examples/*.dg; do \
	  build/$(PROG) $$f -o build/example.plain || { echo "FAIL $$f"; exit 1; }; \
	  cmp -s build/example.plain $${f%.dg}.plain || { echo "DIFFERENT $$f"; exit 1; }; \
	done; echo "examples ok"

install: build/$(PROG)
	install -d $(PREFIX)/bin
	install -m 755 build/$(PROG) $(PREFIX)/bin/$(PROG)
	cd lib && find . -type f \( -name '*.dg' -o -name '*.css' \) | while read f; do \
	  install -d "$(DATADIR)/$$(dirname $$f)" && install -m 644 "$$f" "$(DATADIR)/$$f"; done

uninstall:
	rm -f $(PREFIX)/bin/$(PROG)
	rm -rf $(DATADIR)

clean:
	rm -rf build
