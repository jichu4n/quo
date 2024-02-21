export PATH := $(PATH):$(shell pwd)/node_modules/.bin

.PHONY: all
all: quo2

.PHONY: quo2
quo2: quo2a

.PHONY: quo2a
quo2a: dist/quo2a.wasm

dist/quo2a.wasm: quo1 src/quo2/quo2.quo
	@echo "> Building stage 2a compiler using stage 1c compiler"
	node dist/quo-driver.js 1c dist/quo0-runtime.wat dist/quo1-runtime.quo src/quo2/quo2.quo -o $@

.PHONY: quo1
quo1: quo1c

.PHONY: quo1c
quo1c: dist/quo1c.wasm dist/quo1-runtime.quo

dist/quo1c.wasm: quo1b
	@echo "> Building stage 1c compiler using stage 1b compiler"
	node dist/quo-driver.js 1b dist/quo0-runtime.wat dist/quo1-runtime.quo src/quo1/quo1.quo -o $@

.PHONY: quo1b
quo1b: dist/quo1b.wasm dist/quo1-runtime.quo

dist/quo1b.wasm: quo1a
	@echo "> Building stage 1b compiler using stage 1a compiler"
	node dist/quo-driver.js 1a dist/quo0-runtime.wat dist/quo1-runtime.quo src/quo1/quo1.quo -o $@

.PHONY: quo1a
quo1a: dist/quo1a.wasm dist/quo1-runtime.quo

dist/quo1a.wasm: quo0 dist/quo1-runtime.quo src/quo1/quo1.quo
	@echo "> Building stage 1a compiler using stage 0 compiler"
	node dist/quo-driver.js 0 dist/quo0-runtime.wat dist/quo1-runtime.quo src/quo1/quo1.quo -o $@

dist/quo1-runtime.quo: src/quo1/quo1-runtime.quo
	cp -a $< $@

.PHONY: quo0
quo0: dist/quo0.wasm dist/quo0-runtime.wat dist/quo-driver.js

dist/quo0.wasm: src/quo0/quo0.wat
	@echo "> Building stage 0 compiler"
	@mkdir -p dist
	wat2wasm --enable-exceptions src/quo0/quo0.wat -o $@

dist/quo0-runtime.wat: src/quo0/quo0-runtime.wat
	cp -a $< $@

dist/quo-driver.js: src/*.ts
	tsc

.PHONY: clean
clean:
	rm -rf dist
