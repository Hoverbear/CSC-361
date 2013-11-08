all: clean rdps rdpr

rdps:
	gcc rdps.c -o rdps -Wall -pedantic -lreadline -std=gnu99

rdpr:
	gcc rdps.c -o rdpr -Wall -pedantic -lreadline -std=gnu99

run: clean rdps rdpr
	./rdpr 127.0.0.1 8080 127.0.0.1 8081 tar.gz

clean:
	rm -rf rdps rdpr
