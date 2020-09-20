PROG = robot

FILE1 = main.cpp
FILE2 = sha512.cpp
FILE3 = sha256.cpp

SRCS = $(FILE1) $(FILE2) $(FILE3)

DEBUG=-g
OPTIM=0
#DEBUG=
#OPTIM=fast

current_dir = $(shell pwd)

OPT=-Wall -O$(OPTIM) $(DEBUG) -Wno-write-strings -I../include
LIBS =-L ../lib -lpthread -lcurl -ljansson

CC= g++


all:
	$(CC) $(OPT) $(SRCS) -o $(PROG) $(LIBS)

	mv $(PROG) ../bin

clean:
	rm -rf ../bin/*
