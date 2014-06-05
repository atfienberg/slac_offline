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

makeTemplate: objects/makeTemplate.o
	@echo Linking $@
	$(CC) $(CFLAGS) $^ -o $@ $(ROOTLIBS)

templateFitterTest: objects/templateFitterTest.o
	@echo Linking $@
	$(CC) $(CFLAGS) $^ -o $@ $(ROOTLIBS)

objects/templateFitterTest.o: src/otherExecutables/templateFitterTest.cxx
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
