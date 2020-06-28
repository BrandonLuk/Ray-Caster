CC = gcc
CFLAGS = -std=c11 -lglfw3 -lopengl32 -lgdi32

all: main.c
	$(CC) main.c -o raycaster $(CFLAGS)

clean:
	rm *.exe