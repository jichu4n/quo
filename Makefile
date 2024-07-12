export PATH := $(PATH):$(shell pwd)/node_modules/.bin

# Either "wabt" or "binaryen"
ASSEMBLER ?= binaryen

ifeq ($(ASSEMBLER), wabt)
  wasm_as = wat2wasm --enable-exceptions
  quo_driver_flags = --assembler=wabt
else ifeq ($(ASSEMBLER), binaryen)
  wasm_as = wasm-as --all-features
  quo_driver_flags = --assembler=binaryen
else
  $(error "Invalid assembler: $(ASSEMBLER)")
endif
quo_driver = node dist/quo-driver.js $(quo_driver_flags)

.PHONY: all
all: quo2

.PHONY: quo2
quo2: quo2a

.PHONY: quo2a
quo2a: dist/quo2a.wasm dist/quo2-runtime.quo

dist/quo2a.wasm: quo1 src/quo2/quo2.quo
	@echo "> Building stage 2a compiler using stage 1c compiler"
	$(quo_driver) 1c src/quo2/quo2.quo -o $@

dist/quo2-runtime.quo: src/quo2/quo2-runtime.quo
	cp -a $< $@

.PHONY: quo1
quo1: quo1c

.PHONY: quo1c
quo1c: dist/quo1c.wasm dist/quo1-runtime.quo

dist/quo1c.wasm: quo1b
	@echo "> Building stage 1c compiler using stage 1b compiler"
	$(quo_driver) 1b src/quo1/quo1.quo -o $@

.PHONY: quo1b
quo1b: dist/quo1b.wasm dist/quo1-runtime.quo

dist/quo1b.wasm: quo1a
	@echo "> Building stage 1b compiler using stage 1a compiler"
	$(quo_driver) 1a src/quo1/quo1.quo -o $@

.PHONY: quo1a
quo1a: dist/quo1a.wasm dist/quo1-runtime.quo

dist/quo1a.wasm: quo0 dist/quo1-runtime.quo src/quo1/quo1.quo
	@echo "> Building stage 1a compiler using stage 0 compiler"
	$(quo_driver) 0 dist/quo1-runtime.quo src/quo1/quo1.quo -o $@

dist/quo1-runtime.quo: src/quo1/quo1-runtime.quo
	cp -a $< $@

.PHONY: quo0
quo0: dist/quo0.wasm dist/quo0-runtime.wat dist/quo-driver.js

dist/quo0.wasm: src/quo0/quo0.wat
	@echo "> Building stage 0 compiler"
	@mkdir -p dist
	$(wasm_as) src/quo0/quo0.wat -o $@

dist/quo0-runtime.wat: src/quo0/quo0-runtime.wat
	cp -a $< $@

dist/quo-driver.js: src/*.ts
	tsc

.PHONY: clean
clean:
	rm -rf dist
