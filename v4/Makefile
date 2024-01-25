export PATH := $(PATH):$(shell pwd)/node_modules/.bin

.PHONY: all
all: quo1

.PHONY: quo1
quo1: dist/quo1.wasm

dist/quo1.wasm: quo0 src/quo1.quo
	node dist/quo0.js src/quo1.quo && mv src/quo1.{wat,wasm} dist/

.PHONY: quo0
quo0: dist/quo0.wasm dist/quo0.js

dist/quo0.wasm: src/quo0.wat
	@mkdir -p dist
	wat2wasm --enable-exceptions $< -o $@

dist/quo0.js: src/quo0.ts
	tsc

.PHONY: clean
clean:
	rm -rf dist