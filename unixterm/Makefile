term: terminal.o term.o
	g++ -o term terminal.o term.o

terminal.o: terminal.h
	g++ -c -o terminal.o terminal.c

term.o: terminal.h
	g++ -c -o term.o term.c

clean:
	rm -f term
	rm -f *.o
