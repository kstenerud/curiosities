# Makefile for gcc 2.4.5 on NetBSD-Amiga 1.1

CC	= g++
A_FLAGS	= -g
C_FLAGS	= -Wall $(A_FLAGS)
L_FLAGS	= $(A_FLAGS)
O_FILES	= apply.o area.o cstring.o dir.o extra.o help.o fileio.o flags.o \
          list.o main.o mobile.o object.o reset.o room.o shop.o special.o \
          zone.o

merc2rom: $(O_FILES)
	rm -f merc2rom
	$(CC) $(L_FLAGS) -o merc2rom $(O_FILES)

.c.o:
	$(CC) -c $(C_FLAGS) $<
