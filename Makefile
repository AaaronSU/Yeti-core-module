all: run


run: mkdir_bin
	gcc vm.c -o bin/vm
	./bin/vm reference/hello_world.archyb
	./bin/vm reference/fibonacci.archyb

vm_debug: mkdir_bin
	gcc vm.c -o bin/vm -DDEBUG
	./bin/vm reference/hello_world.archyb
	./bin/vm reference/fibonacci.archyb

verify_as: mkdir_bin
	gcc as.c -o bin/as -Werror -O3 -Wall -Wconversion -Wextra
	./bin/as reference/hello_world.asm
	python3 script/compare.py output.archyb reference/hello_world.archyb
	./bin/as reference/fibonacci.asm
	python3 script/compare.py output.archyb reference/fibonacci.archyb
	# ./bin/as reference/dotprod.asm

mkdir_bin:
	mkdir -p bin

clean:
	rm bin/header_reader
	rm bin/as