CFLAGS:= -std=c++14 -Wall $(shell ncursesw5-config --cflags)
LFLAGS:= $(shell ncursesw5-config --libs) -L lib/vcd_parser -lverilog-vcd-parser

default: build_lib termvcd

build_lib: lib/vcd_parser/libverilog-vcd-parser.a

lib/vcd_parser/libverilog-vcd-parser.a:
	-git clone https://github.com/ben-marshall/verilog-vcd-parser.git
	-make -C verilog-vcd-parser vcd-parser build/libverilog-vcd-parser.a
	-mkdir -p lib/vcd_parser/
	-cp verilog-vcd-parser/build/* lib/vcd_parser/
	# -rm -rf verilog-vcd-parser


termvcd: termvcd.o lib/vcd_parser/libverilog-vcd-parser.a
	g++ $(CFLAGS) -o $@ $< $(LFLAGS)

termvcd.o: src/main.cpp
	g++ -c $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	make -C verilog-vcd-parser clean
	rm termvcd termvcd.o
