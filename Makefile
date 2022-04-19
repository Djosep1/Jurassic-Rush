all: game

game: game.cpp
	g++ game.cpp timers.cpp libggfonts.a -Wall -ogame -lX11 -lGL -lGLU -lm -lpthread

clean:
	rm -f game
