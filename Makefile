SUBDIR := scanner_generator
SPEC ?= regexes_ex.txt

.PHONY: all run debug clean

all:
	$(MAKE) -C $(SUBDIR) all

run:
	$(MAKE) -C $(SUBDIR) run SPEC=$(SPEC)

debug:
	$(MAKE) -C $(SUBDIR) debug SPEC=$(SPEC)

clean:
	$(MAKE) -C $(SUBDIR) clean