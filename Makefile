default: termvcd

build_lib: lib/libverilog-vcd-parser.a

lib/vcd_parser/libverilog-vcd-parser.a:
	-git clone https://github.com/ben-marshall/verilog-vcd-parser.git
	-make -C verilog-vcd-parser vcd-parser build/libverilog-vcd-parser.a
	-mkdir -p lib/vcd_parser/
	-cp verilog-vcd-parser/build/* lib/vcd_parser/
	# -rm -rf verilog-vcd-parser


termvcd: lib/vcd_parser/libverilog-vcd-parser.a termvcd.o
	g++ -Wall -o $@ $^ -lncurses -L lib/vcd_parser -lverilog-vcd-parser

termvcd.o: src/main.cpp
	g++ -c -Wall -o $@ $^

.PHONY: clean
clean:
	make -C verilog-vcd-parser clean
	rm termvcd termvcd.o
