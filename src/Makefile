all:prog clean

prog:	scanner.l parser.y
	flex -o lex.yy.cpp scanner.l
	bison -d -o parser.tab.cpp parser.y
	g++ -o a lex.yy.cpp parser.tab.cpp main.cpp -lm

clean:
	rm lex.yy.cpp parser.tab.cpp parser.tab.hpp
