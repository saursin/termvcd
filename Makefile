NCURSES_VERSION := 6.5
NCURSES_URL     := https://ftp.gnu.org/pub/gnu/ncurses/ncurses-$(NCURSES_VERSION).tar.gz
NCURSES_PREFIX  := $(CURDIR)/lib/ncurses

NCURSES_CFLAGS  := -I$(NCURSES_PREFIX)/include/ncursesw -I$(NCURSES_PREFIX)/include
NCURSES_LFLAGS  := -L$(NCURSES_PREFIX)/lib -lncursesw

VCD_PARSER_LIB  := lib/vcd_parser/libverilog-vcd-parser.a

CFLAGS := -std=c++14 -Wall -DNCURSES_ENABLE_STDBOOL_H=1 $(NCURSES_CFLAGS)
LFLAGS := $(NCURSES_LFLAGS) -L lib/vcd_parser -lverilog-vcd-parser

default: build_lib termvcd

build_lib: $(VCD_PARSER_LIB) $(NCURSES_PREFIX)/lib/libncursesw.a

# ── ncurses ──────────────────────────────────────────────────────────────────

$(NCURSES_PREFIX)/lib/libncursesw.a: ncurses-$(NCURSES_VERSION).tar.gz
	tar xf $<
	cd ncurses-$(NCURSES_VERSION) && \
	  ./configure --prefix=$(NCURSES_PREFIX) \
	              --enable-widec \
	              --with-normal \
	              --without-shared \
	              --without-debug \
	              --without-manpages \
	              --without-progs \
	              --without-tests \
	              --without-cxx-binding && \
	  make -j$$(nproc) && \
	  make install
	rm -rf ncurses-$(NCURSES_VERSION)

ncurses-$(NCURSES_VERSION).tar.gz:
	curl -fLO $(NCURSES_URL)

# ── vcd-parser ───────────────────────────────────────────────────────────────

$(VCD_PARSER_LIB):
	-git clone https://github.com/ben-marshall/verilog-vcd-parser.git
	-make -C verilog-vcd-parser vcd-parser build/libverilog-vcd-parser.a
	-mkdir -p lib/vcd_parser/
	-cp verilog-vcd-parser/build/* lib/vcd_parser/

# ── main target ──────────────────────────────────────────────────────────────

termvcd: termvcd.o $(VCD_PARSER_LIB) $(NCURSES_PREFIX)/lib/libncursesw.a
	g++ $(CFLAGS) -o $@ $< $(LFLAGS)

termvcd.o: src/main.cpp
	g++ -c $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	make -C verilog-vcd-parser clean
	rm -f termvcd termvcd.o
