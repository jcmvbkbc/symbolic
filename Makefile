all: sha1 sha12 sha13
clean:
	rm -f sha1 sha12 sha13

SRC := sha1.cpp
DEP := sha1.cpp symbolic.h Makefile
OPT := -W -Wall -std=c++11 -g3

sha1: $(DEP)
	g++ -O0 $(OPT) $(SRC) -o $@
sha12: $(DEP)
	g++ -O2 $(OPT) $(SRC) -o $@
sha13: $(DEP)
	g++ -O3 $(OPT) $(SRC) -o $@
