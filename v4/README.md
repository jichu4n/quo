# Bootstrapped Quo -> WebAssembly compiler

## Stages

- **Stage 0 ([quo0.wat](./src/quo0.wat)):** Written in WebAssembly, compiles a minimal subset of Quo (Quo0) in which stage 1 compiler is written.
  - Single pass
  - Minimal Turing complete syntax
  - Only data type is i32
  - No memory management
- **Stage 1 ([quo1.quo](./src/quo1.quo)):** Written in Quo0 and compiles Quo0. It is effectively a translation of the stage 0 compiler into Quo0, with no additional language features added.
  - `quo1a.wasm` is the result of compiling stage 1 compiler using stage 0 compiler
  - `quo1b.wasm` is the result of compiling stage 1 compiler using itself
  - Quality-of-life improvements in the compiler implementation:
    - Simple memory management malloc / free
    - Basic data structures like dynamic strings, arrays and maps
- **Stage 2-n:** Written in increasingly advanced versions of Quo, with each stage able to compile itself and the next stage.
