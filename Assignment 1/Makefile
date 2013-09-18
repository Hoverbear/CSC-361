all: clean sws

run: clean sws
	./sws 8080 stuff

sws:
	gcc --ansi sws.c -o sws -pthread -Wall

clean:
	rm -rf *.out sws