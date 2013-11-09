all: clean rdps rdpr

rdps:
	gcc rdps.c resources.c -o rdps -Wall -pedantic -lreadline -std=gnu99 -g -I.

rdpr:
	gcc rdpr.c resources.c -o rdpr -Wall -pedantic -lreadline -std=gnu99 -g -I.

run: clean rdps rdpr
	./rdpr 127.0.0.1 8081 test_destination &
	./rdps 127.0.0.1 8080 127.0.0.1 8081 tests/beowulf
	rm test_destination

grind: clean rdps rdpr
	valgrind --track-origins=yes ./rdpr 127.0.0.1 8081 test_destination &
	valgrind --track-origins=yes ./rdps 127.0.0.1 8080 127.0.0.1 8081 test_file

clean:
	rm -rf rdps rdpr
