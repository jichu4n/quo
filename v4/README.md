# Bootstrapped Quo -> WebAssembly compiler

## Stages

- **Stage 0 ([quo0.wat](./src/quo0.wat)):** Written in WebAssembly, supports a minimal subset of Quo. Compiles stage 1 compiler.
  - Single pass
  - Minimal Turing complete syntax
  - Only data type is raw i32 values
  - No garbage collection
- **Stage 1 ([quo1.quo](./src/quo1.quo)):** Written in minimal subset of Quo supported by stage 0, supports a more advanced version of Quo. Compiles itself and stage 2 compiler.
  - All values stored on heap
  - Supports strings and arrays
- **Stage 2-n:** Written in increasingly advanced versions of Quo, with each stage able to compile itself and the next stage.
