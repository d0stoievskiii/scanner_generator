CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic

APP := scanner
DBG_APP := scanner_debug
SPEC ?= regexes_ex.txt
INPUT ?= input.txt
OUT ?= tokens.json

.PHONY: all clean run debug tokenize

all: $(APP) $(DBG_APP)

$(APP): main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(DBG_APP): debug_main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

run: $(APP)
	./$(APP) $(SPEC)

tokenize: $(APP)
	./$(APP) $(SPEC) $(INPUT) $(OUT)

debug: $(DBG_APP)
	./$(DBG_APP) $(SPEC)

clean:
	rm -f $(APP) $(DBG_APP)