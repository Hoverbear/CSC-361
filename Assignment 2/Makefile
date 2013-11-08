all: clean rdps rdpr

rdps:
	gcc rdps.c -o rdps -Wall -pedantic -lreadline -std=gnu99

rdpr:
	gcc rdps.c -o rdpr -Wall -pedantic -lreadline -std=gnu99

run: rdps rdpr
	echo "Hello"

clean:
	rm -rf rdps rdpr
