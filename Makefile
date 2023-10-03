CC = mpic++
FLAGS = -Wall -O0 -g
LIBS = -pthread
SOURCE = *.cpp *.h
TARGET = test

all : $(SOURCE)
	$(CC) $(FLAGS) $^ -o $(TARGET) $(LIBS)
clean : 
	rm $(TARGET) -rf