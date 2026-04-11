CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic

APP := scanner
DBG_APP := scanner_debug
TOKENS_APP := scanner_tokens_json
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
	rm -f $(APP) $(DBG_APP) $(TOKENS_APP)