CC = gcc
CFLAGS = -W -Wall
TARGET = _bpt
OBJECTS = utility.o file.o buffer.o open.o close.o find.o insert.o delete.o main.o

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

utility.o : utility.c
	$(CC) $(CFALGS) -c -o utility.o utility.c

file.o : file.c 
	$(CC) $(CFALGS) -c -o file.o file.c

buffer.o : buffer.c 
	$(CC) $(CFALGS) -c -o buffer.o buffer.c

open.o : open.c
	$(CC) $(CFALGS) -c -o open.o open.c

close.o : close.c
	$(CC) $(CFALGS) -c -o close.o close.c

find.o : find.c
	$(CC) $(CFALGS) -c -o find.o find.c

insert.o : insert.c
	$(CC) $(CFALGS) -c -o insert.o insert.c 

delete.o : delete.c
	$(CC) $(CFALGS) -c -o delete.o delete.c

main.o : main.c
	$(CC) $(CFALGS) -c -o main.o main.c

clean :
	rm -f $(OBJECTS) $(TARGET)