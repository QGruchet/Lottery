compile: clean
	gcc -Wall -g million.c -o million

valgrind: compile
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./million


clean:
	rm -f million
	ls -l