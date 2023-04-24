all: gif
gif: gif.c
	gcc gif.c -o gif
clean:
	rm -f gif
