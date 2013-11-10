all: clean rdp

rdp:
	gcc rdp.c resources.c -o rdp -Wall -pedantic -lreadline -lpthread -std=gnu99 -g -I.

run: clean rdp
	./rdp 127.0.0.1 8081 test_destination &
	./rdp 127.0.0.1 8080 127.0.0.1 8081 tests/beowulf
	rm test_destination

grind: clean rdp
	valgrind --track-origins=yes ./rdp 127.0.0.1 8081 test_destination &
	valgrind --track-origins=yes ./rdp 127.0.0.1 8080 127.0.0.1 8081 tests/beowulf 

clean:
	rm -rf rdp test_destination
