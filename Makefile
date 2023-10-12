CC = mpic++
FLAGS = -Wall -O0 -g
LIBS = -pthread
INCLUDES = -I ./include
SOURCE = src/*.cpp
TARGET = sim_proc

all : $(SOURCE)
	$(CC) $(INCLUDES) $(FLAGS) $^ -o $(TARGET) $(LIBS)
clean : 
	rm $(TARGET) -rf