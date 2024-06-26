all: run_par

run_par: mkdir_bin
	gcc vm.c main.c -o bin/vm -Wall -Wconversion -Wextra -g3
	./bin/vm script.conf

run: mkdir_bin
	#gcc vm.c main.c -o bin/vm -Wall -Wconversion -Wextra
	gcc main.c vm.c -o bin/vm -Wall -Wconversion -Wextra
	./bin/vm script.conf
	# ./bin/vm reference/hello_world.archyb
	# ./bin/vm reference/fibonacci.archyb
	# ./bin/vm reference/dotprod_u64.archyb


vm_debug: mkdir_bin
	# gcc vm.c -o bin/vm -DDEBUG -Wall -Wconversion -Wextra
	gcc main.c vm.c -o bin/vm -DDEBUG -Wall -Wconversion -Wextra
	./bin/vm script.conf
	# ./bin/vm reference/hello_world.archyb
	# ./bin/vm reference/fibonacci.archyb
	# ./bin/vm reference/dotprod_u64.archyb


verify_as: mkdir_bin
	gcc as.c -o bin/as -Werror -O3 -Wall -Wconversion -Wextra
	./bin/as reference/hello_world.asm
	python3 script/compare.py output.archyb reference/hello_world.archyb
	./bin/as reference/fibonacci.asm
	python3 script/compare.py output.archyb reference/fibonacci.archyb
	./bin/as reference/dotprod_u64.asm

mkdir_bin:
	mkdir -p bin

bench: bench.c
	gcc bench.c vm.c -o bin/bench -lm -g
	./bin/bench 

.PHONY: clean check

clean:
	rm -f bin/*

check: test.c
	gcc test.c vm.c -o bin/test -lcmocka -lm -g3
	./bin/test 