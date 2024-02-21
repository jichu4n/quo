# Quo: Toy self-hosted language

Quo is a toy language with C-style syntax that compiles to WebAssembly. It is
bootstrapped with a minimal compiler implemented in hand-written WebAssembly.
Each subsequent iteration of the compiler is written in the subset of the
language supported by the previous iteration, and provides additional language
features for the next iteration.

## Why?

Because it's a fun challenge! I've always been interested in compilers and
wanted to learn more about WebAssembly. There's just something very cool about
bootstrapping a self-hosted compiler.

## Iterations

- **Stage 0 ([quo0.wat](./src/quo0/quo0.wat)):** Written in WebAssembly,
  compiles a minimal version of Quo (Quo0) in which stage 1 compiler is
  written.
  - Single pass, no AST / IR
  - Minimal Turing complete syntax
  - Only data type is i32
  - Uses linear memory with no memory management
- **Stage 1 ([quo1.quo](./src/quo1/quo1.quo)):** Written in Quo0, compiles
  Quo0. Effectively a translation of the stage 0 compiler into Quo0 itself,
  with no additional language features added.
  - `quo1a.wasm`: stage 1 compiler compiled by stage 0 compiler
  - `quo1b.wasm`: stage 1 compiler compiled by `quo1a.wasm`
  - `quo1c.wasm`: stage 1 compiler compiled by `quo1b.wasm`, i.e. a fully
    self-hosted compiler!
  - Includes runtime providing some basic runtime functions:
    - Memory management with malloc / free
    - Data structures including dynamic strings, arrays and maps
- **(WIP) Stage 2 ([quo2.quo](./src/quo2/quo2.quo)):** Written in Quo0,
  compiles a more advanced version of Quo (Quo1) supporting data types that
  compile to WasmGC `struct`s.

