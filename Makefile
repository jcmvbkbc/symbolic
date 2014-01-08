all: sha1 sha12 sha13

sha1: sha1.cpp symbolic.h
	g++ -O0 -W -Wall -std=c++11 -g3 sha1.cpp -o sha1
sha12: sha1.cpp symbolic.h
	g++ -O2 -W -Wall -std=c++11 -g3 sha1.cpp -o sha12
sha13: sha1.cpp symbolic.h
	g++ -O3 -W -Wall -std=c++11 -g3 sha1.cpp -o sha13
