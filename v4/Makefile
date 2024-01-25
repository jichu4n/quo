export PATH := $(PATH):$(shell pwd)/node_modules/.bin

.PHONY: all
all: quo1

.PHONY: quo1
quo1: dist/quo1b.wasm

dist/quo1b.wasm: dist/quo1a.wasm src/quo1.quo
	@echo "> Building stage 1b compiler using stage 1a compiler"
	node dist/quo-driver.js 1a src/quo1.quo dist/quo1b.wasm

dist/quo1a.wasm: quo0 src/quo1.quo
	@echo "> Building stage 1a compiler using stage 0 compiler"
	node dist/quo-driver.js 0 src/quo1.quo dist/quo1a.wasm

.PHONY: quo0
quo0: dist/quo0.wasm dist/quo-driver.js

dist/quo0.wasm: src/quo0.wat
	@echo "> Building stage 0 compiler"
	@mkdir -p dist
	wat2wasm --enable-exceptions $< -o $@

dist/quo-driver.js: src/*.ts
	tsc

.PHONY: clean
clean:
	rm -rf dist