all: clean sws

run: clean sws
	./sws 8080 stuff
	# echo -n "GET / HTTP/1.0\r\n\r\n" | nc -u 127.0.0.1 8080

sws:
	gcc --ansi sws.c -o sws -pthread -Wall

clean:
	rm -rf *.out sws
