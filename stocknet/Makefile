INSTALLDIR = /usr/local/stocknet

stocknet: stock.o player.o timer.o textstr.o socklist.o cmds.o game.o
	g++ -g -o stocknet game.o player.o timer.o textstr.o socklist.o cmds.o stock.o

stock.o:
	g++ -g -c -o stock.o stock.c

game.o:
	g++ -g -c -o game.o game.c

timer.o: timer.h
	g++ -g -c -o timer.o timer.c

textstr.o: textstr.h
	g++ -g -c -o textstr.o textstr.c

socklist.o: socklist.h textstr.h
	g++ -g -c -o socklist.o socklist.c

server.o: socklist.h timer.h textstr.h cmds.h
	g++ -g -c -o server.o server.c

cmds.o: cmds.h textstr.h
	g++ -g -c -o cmds.o cmds.c

player.o: player.h
	g++ -g -c -o player.o player.c

clean:
	rm -f *.o stocknet core

install: stocknet
	mkdir -p $(INSTALLDIR)/players $(INSTALLDIR)/admin
	cp -f stocknet $(INSTALLDIR)/.

	