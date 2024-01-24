import fs from 'fs-extra';
import path from 'path';
import wabt from 'wabt';

const quo0 = 'quo0.wasm';

function getWasmString(memory: WebAssembly.Memory, ptr: number) {
  const bytes = new Uint8Array(memory.buffer, ptr);
  const len = bytes.findIndex((byte) => byte === 0);
  return new TextDecoder().decode(new Uint8Array(memory.buffer, ptr, len));
}

function setWasmString(memory: WebAssembly.Memory, ptr: number, str: string) {
  new TextEncoder().encodeInto(str, new Uint8Array(memory.buffer, ptr));
}

const wasmExceptionTag = new WebAssembly.Tag({parameters: ['i32']});

async function setupWasmModule() {
  const wasmFile = await fs.readFile(
    path.join(__dirname, '..', 'dist', quo0)
  );
  const wasmMemory = new WebAssembly.Memory({initial: 256});
  const wasmModule = await WebAssembly.instantiate(wasmFile, {
    env: {memory: wasmMemory, tag: wasmExceptionTag},
    debug: {
      puts(ptr: number) {
        console.log(getWasmString(wasmMemory, ptr));
      },
    },
  });
  return {wasmModule, wasmMemory};
}

class WebAssemblyError extends Error {
  constructor(wasmMemory: WebAssembly.Memory, e: unknown) {
    if (e instanceof WebAssembly.Exception && e.is(wasmExceptionTag)) {
      super(getWasmString(wasmMemory, e.getArg(wasmExceptionTag, 0)));
    } else if (e instanceof Error) {
      super(e.message);
      this.stack = e.stack;
    } else {
      super(String(e));
    }
    this.name = this.constructor.name;
  }
}

export interface Token {
  type: number;
  value?: number | string;
}

export async function tokenize(input: string): Promise<Array<Token>> {
  const {wasmModule, wasmMemory} = await setupWasmModule();
  const init = wasmModule.instance.exports.init as CallableFunction;
  const nextToken = wasmModule.instance.exports.nextToken as CallableFunction;

  setWasmString(wasmMemory, 0, input);
  const tokenValuePtr = 4096;
  const tokens: Array<Token> = [];
  try {
    init(0);
    do {
      const tokenType = nextToken(tokenValuePtr);
      const token = {
        type: tokenType,
        ...(tokenType === 0
          ? {}
          : {value: getWasmString(wasmMemory, tokenValuePtr)}),
      };
      tokens.push(token);
    } while (tokens[tokens.length - 1].type !== 0);
  } catch (e) {
    throw new WebAssemblyError(wasmMemory, e);
  }
  return tokens;
}

async function runCompileFn(
  compileFn: (exports: WebAssembly.Exports) => number,
  input: string
): Promise<string> {
  const {wasmModule, wasmMemory} = await setupWasmModule();
  const init = wasmModule.instance.exports.init as CallableFunction;
  setWasmString(wasmMemory, 0, input);
  try {
    init(0);
    return getWasmString(wasmMemory, compileFn(wasmModule.instance.exports));
  } catch (e) {
    throw new WebAssemblyError(wasmMemory, e);
  }
}

export async function compileExpr(input: string): Promise<string> {
  return await runCompileFn(
    ({compileExpr}) => (compileExpr as CallableFunction)(),
    input
  );
}

export async function compileStmt(input: string): Promise<string> {
  return await runCompileFn(
    ({compileStmt}) => (compileStmt as CallableFunction)(),
    input
  );
}

export async function compileBlock(input: string): Promise<string> {
  return await runCompileFn(
    ({compileBlock}) => (compileBlock as CallableFunction)(0),
    input
  );
}

export async function compileFn(input: string): Promise<string> {
  return await runCompileFn(
    ({compileFn}) => (compileFn as CallableFunction)(),
    input
  );
}

export async function compileModule(input: string): Promise<string> {
  return await runCompileFn(
    ({compileModule}) => (compileModule as CallableFunction)(),
    input
  );
}

export async function compileQuoFile(inputFile: string) {
  const input = await fs.readFile(inputFile, 'utf8');
  const watOutput = await compileModule(input);
  const watOutputFile = path.format({
    ...path.parse(inputFile),
    base: '',
    ext: '.wat',
  });
  await fs.writeFile(watOutputFile, watOutput);
  const wasmModule = (await wabt()).parseWat(watOutputFile, watOutput);
  const wasmOutputFile = path.format({
    ...path.parse(inputFile),
    base: '',
    ext: '.wasm',
  });
  await fs.writeFile(wasmOutputFile, wasmModule.toBinary({}).buffer);
  return {watOutputFile, wasmOutputFile};
}

if (require.main === module) {
  (async () => {
    if (process.argv.length < 3) {
      console.error('Usage: quo0 <input>');
      process.exit(1);
    }
    await compileQuoFile(process.argv[2]);
  })();
}
