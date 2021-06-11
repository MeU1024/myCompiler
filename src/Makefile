all:prog clean

prog:	scanner.l parser.y 
	bison -d parser.y -o parser.cpp
	flex -o scanner.cpp scanner.l
	g++ -c scanner.cpp -o scanner.o
	g++ -c parser.cpp -o parser.o
	g++ -c Vis.cpp -o Vis.o
	g++ -c main.cpp -o main.o
	g++ scanner.o parser.o Vis.o main.o -o main

clean:
	rm lex.yy.cpp parser.tab.cpp parser.tab.hpp
