all: mandel

mandel: Mandelbrot.c
	mpicc Mandelbrot.c -o mandel -lm

clean:
	rm -f mandel mandel.txt
