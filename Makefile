CC = g++ -I. -I./src #Compiler
CFLAGS = -Wall -std=c++0x $(shell root-config --cflags)#Compiler Flags
SOURCES = $(wildcard src/*.cxx) #Source Code Files
OBJECTS = $(patsubst src/%.cxx,objects/%.o,$(SOURCES)) #Objects
ROOTLIBS = $(shell root-config --libs) 
ROOT_DICT = objects/root_dict.o
TARGET =  exampleDriver

$(TARGET): $(OBJECTS) 
	@echo Linking $@
	$(CC) $(CFLAGS) $^ -o $@ $(ROOTLIBS)

objects/%.o: src/%.cxx
	@echo Building $@
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm objects/*

distclean:
	rm $(OBJECTS) $(TARGET)