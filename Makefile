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

CFLAGS = -I. -I./src -Wall -std=c++0x $(shell root-config --cflags)
SOURCES = $(wildcard src/*.cxx) #Source Code Files
OBJECTS = $(patsubst src/%.cxx,objects/%.o,$(SOURCES)) #Objects
ROOTLIBS = $(shell root-config --libs) 
ROOT_DICT = objects/root_dict.o
TARGET =  exampleDriver

$(TARGET): $(OBJECTS) 
	@echo Linking $@
	$(CC) $(CFLAGS) $^ -o $@ $(ROOTLIBS)

makeTemplate: objects/makeTemplate.o
	@echo Linking $@
	$(CC) $(CFLAGS) $^ -o $@ $(ROOTLIBS)

templateFitterTest: objects/templateFitterTest.o objects/pulseFitter.o
	@echo Linking $@
	$(CC) $(CFLAGS) $^ -o $@ $(ROOTLIBS)

objects/templateFitterTest.o: src/otherExecutables/templateFitterTest.cxx objects/pulseFitter.o
	@echo Building $@
	$(CC) $(CFLAGS) $< -c -o $@

objects/makeTemplate.o: src/otherExecutables/makeTemplate.cxx
	@echo Building $@
	$(CC) $(CFLAGS) $< -c -o $@

objects/%.o: src/%.cxx
	@echo Building $@
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm objects/*

distclean:
	rm $(OBJECTS) $(TARGET)
