export PATH := $(PATH):$(shell pwd)/node_modules/.bin

.PHONY: all
all: quo0

.PHONY: quo0
quo0: dist/quo0.wasm dist/quo0.js

dist/quo0.wasm: src/quo0.wat
	mkdir -p dist
	wat2wasm --enable-exceptions $< -o $@

dist/quo0.js: src/quo0.ts
	tsc

.PHONY: clean
clean:
	rm -rf dist