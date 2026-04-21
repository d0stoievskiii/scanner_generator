ifeq ($(OS),Windows_NT)
    RM = del /Q /F
    EXT = .exe
    FIX_PATH = $(subst /,\,$1)
else
    RM = rm -f
    EXT =
    FIX_PATH = $1
endif

CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic

APP := scanner$(EXT)
DBG_APP := scanner_debug$(EXT)
TOKENS_APP := scanner_tokens_json$(EXT)

SPEC ?= regexes_ex.txt
AFD_OUT ?= afd.json

.PHONY: all clean run debug export-afd export-tokens

all: $(APP) $(DBG_APP) $(TOKENS_APP)

$(APP): main.cpp generate_scanner_code.cpp
	$(CXX) $(CXXFLAGS) main.cpp generate_scanner_code.cpp -o $@

$(DBG_APP): debug_main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(TOKENS_APP): export_tokens_json.cpp out_scanner/scanner_gerado.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

run: $(APP)
	./$(APP) $(SPEC) $(AFD_OUT)

export-afd: $(APP)
	./$(APP) $(SPEC) $(AFD_OUT)

export-tokens: $(TOKENS_APP)
	./$(TOKENS_APP)

debug: $(DBG_APP)
	./$(DBG_APP) $(SPEC)

clean:
	$(RM) $(APP) $(DBG_APP) $(TOKENS_APP)