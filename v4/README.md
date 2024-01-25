# Bootstrapped Quo -> WebAssembly compiler

## Stages

- **Stage 0 ([quo0.wat](./src/quo0.wat)):** Written in WebAssembly, supports a minimal subset of Quo. Compiles stage 1 compiler.
  - Single pass
  - Provides minimal Turing complete syntax
  - Uses and provides raw i32 values as sole data type
  - Uses null-terminated strings
  - Does not use / provide memory management
- **Stage 1 ([quo1.quo](./src/quo1.quo)):** Written in minimal subset of Quo supported by stage 0, supports a more advanced version of Quo. Compiles itself and stage 2 compiler.
  - `quo1a.wasm` is the result of compiling stage 1 code using stage 0 compiler
  - `quo1b.wasm` is the result of compiling stage 1 code using itself
  - Uses first-fit malloc / free for memory management
  - Uses linked list of chunks as string representation
  - Provides strings, structs and arrays using WasmGC
- **Stage 2-n:** Written in increasingly advanced versions of Quo, with each stage able to compile itself and the next stage.
