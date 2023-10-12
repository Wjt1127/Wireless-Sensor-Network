CC = mpic++
FLAGS = -Wall -O0 -g
LIBS = -pthread
SOURCE = *.cpp *.h
TARGET = sim_proc

all : $(SOURCE)
	$(CC) $(FLAGS) $^ -o $(TARGET) $(LIBS)
clean : 
	rm $(TARGET) -rf