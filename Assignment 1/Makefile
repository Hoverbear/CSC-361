# echo -e -n "GET / HTTP/1.0\r\n\r\n" | nc -u 127.0.0.1 8080
all: clean sws

run: clean sws
	./sws 8080 www

sws:
	gcc --ansi sws.c -o sws -pthread -Wall

clean:
	chmod 200 www/cantRead
	rm -rf *.out sws