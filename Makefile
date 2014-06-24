# Figure out the architecture
UNAME_S = $(shell uname -s)

# Clang compiler
ifeq ($(UNAME_S), Darwin)
	CC = clang++
endif

# Gnu compiler
ifeq ($(UNAME_S), Linux)
	CC = g++
endif

CFLAGS = -I. -I./src -Wall -std=c++0x $(shell root-config --cflags) -O3 -ffast-math -march=native -pipe
SOURCES = $(wildcard src/*.cxx) #Source Code Files
OBJECTS = $(patsubst src/%.cxx,objects/%.o,$(SOURCES)) #Objects
ROOTLIBS = $(shell root-config --libs) 
ROOT_DICT = objects/root_dict.o
TARGET =  slacAnalyzer

$(TARGET): $(OBJECTS) 
	@echo Linking $@
	$(CC) $(CFLAGS) $^ -o $@ $(ROOTLIBS)

%: src/otherExecutables/%.cxx objects/pulseFitter.o
	@echo Linking and building $@
	$(CC) $(CFLAGS) $^ -o $@ $(ROOTLIBS)

objects/%.o: src/%.cxx objects/
	@echo Building $@
	$(CC) $(CFLAGS) $< -c -o $@

objects/:
	mkdir objects

clean:
	rm objects/*

distclean:
	rm $(OBJECTS) $(TARGET)
