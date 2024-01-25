export PATH := $(PATH):$(shell pwd)/node_modules/.bin

.PHONY: all
all: quo1

.PHONY: quo2
quo2: dist/quo2.wasm

dist/quo2.wasm: quo1 src/quo1.quo
	node dist/quo-driver.js 1 src/quo1.quo dist/quo2.wasm

.PHONY: quo1
quo1: dist/quo1.wasm

dist/quo1.wasm: quo0 src/quo1.quo
	node dist/quo-driver.js 0 src/quo1.quo dist/quo1.wasm

.PHONY: quo0
quo0: dist/quo0.wasm dist/quo-driver.js

dist/quo0.wasm: src/quo0.wat
	@mkdir -p dist
	wat2wasm --enable-exceptions $< -o $@

dist/quo-driver.js: src/*.ts
	tsc

.PHONY: clean
clean:
	rm -rf dist