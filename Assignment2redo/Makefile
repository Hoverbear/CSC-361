all: clean rdp

rdp:
	gcc rdps.c shared.c -o rdps -Wall -pedantic -lreadline -lpthread -std=gnu99 -g -I.
	gcc rdpr.c shared.c -o rdpr -Wall -pedantic -lreadline -lpthread -std=gnu99 -g -I.

run: clean rdp
	./rdpr 127.0.0.1 8081 test_destination &
	./rdps 127.0.0.1 8080 127.0.0.1 8081 tests/beowulf
	rm test_destination

grind: clean rdp
	valgrind --track-origins=yes ./rdpr 127.0.0.1 8081 test_destination &
	valgrind --track-origins=yes ./rdps 127.0.0.1 8080 127.0.0.1 8081 tests/beowulf 

clean:
	rm -rf rdpr rdps test_destination
