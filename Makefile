run:
	mkdir -p bin
	gcc as.c -o bin/as -Werror -O3
	./bin/as ./example/hello_world.asm
	python3 compare.py try.archy example/parallele_hello_world.archy

clean:
	rm bin/header_reader
	rm bin/as