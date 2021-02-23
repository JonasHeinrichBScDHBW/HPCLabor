# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS = -g -O2 -Wall -std=c99 -lc -lm -fopenmp -D _DEFAULT_SOURCE -march=native

# the build target executable:
TARGET_GAME_OF_LIFE = gameoflife
TARGET_TEST = test

all: $(TARGET_GAME_OF_LIFE)
test: $(TARGET_TEST)

$(TARGET_GAME_OF_LIFE): $(TARGET_GAME_OF_LIFE).c
	$(CC) -o $(TARGET_GAME_OF_LIFE) $(TARGET_GAME_OF_LIFE).c $(CFLAGS)

$(TARGET_TEST): $(TARGET_TEST).c
	$(CC) -o $(TARGET_TEST) $(TARGET_TEST).c $(CFLAGS)

run: all
	./$(TARGET_GAME_OF_LIFE)

clean:
	$(RM) $(TARGET_GAME_OF_LIFE)
