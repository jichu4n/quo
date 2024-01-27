import fs from 'fs-extra';
import path from 'path';
import wabt from 'wabt';

export function getWasmString(memory: WebAssembly.Memory, ptr: number) {
  const bytes = new Uint8Array(memory.buffer, ptr);
  const len = bytes.findIndex((byte) => byte === 0);
  return new TextDecoder().decode(new Uint8Array(memory.buffer, ptr, len));
}

export function setWasmString(
  memory: WebAssembly.Memory,
  ptr: number,
  str: string
) {
  const buffer = new Uint8Array(memory.buffer, ptr);
  const len = new TextEncoder().encodeInto(str, buffer).written;
  buffer[ptr + len] = 0;
  return len + 1;
}

const wasmExceptionTag = new WebAssembly.Tag({parameters: ['i32']});

export async function setupWasmModule(stage: string) {
  const wasmFile = await fs.readFile(
    path.join(__dirname, '..', 'dist', `quo${stage}.wasm`)
  );
  const wasmMemory = new WebAssembly.Memory({initial: 256});
  const wasmModule = await WebAssembly.instantiate(wasmFile, {
    env: {memory: wasmMemory, tag: wasmExceptionTag},
    debug: {
      puts(ptr: number): number {
        console.log(getWasmString(wasmMemory, ptr));
        return 0;
      },
      putn(n: number): number {
        console.log(n);
        return 0;
      },
    },
  });
  const fns = Object.fromEntries(
    Object.entries(wasmModule.instance.exports)
      .filter(([name, value]) => typeof value === 'function')
      .map(([name, value]) => [
        name,
        wrapWebAssemblyFn(wasmMemory, value as Function),
      ])
  );
  return {wasmModule, wasmMemory, fns};
}

export async function getWatRuntimeString(stage: string) {
  const watFilePath = path.join(
    __dirname,
    '..',
    'dist',
    `quo${stage}-runtime.wat`
  );
  if (!(await fs.exists(watFilePath))) {
    return '';
  }
  return await fs.readFile(watFilePath, 'utf8');
}

export class WebAssemblyError extends Error {
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

function wrapWebAssemblyFn(
  wasmMemory: WebAssembly.Memory,
  fn: CallableFunction
): CallableFunction {
  return (...args: Array<unknown>) => {
    try {
      return fn(...args);
    } catch (e) {
      throw new WebAssemblyError(wasmMemory, e);
    }
  };
}

export async function compileModule(
  stage: string,
  input: string
): Promise<string> {
  const {wasmMemory, fns} = await setupWasmModule(stage);
  const watRuntimeString = await getWatRuntimeString(stage);
  const {init, compileModule} = fns;
  const len = setWasmString(wasmMemory, 0, input);
  setWasmString(wasmMemory, len, watRuntimeString);
  init(0);
  return getWasmString(wasmMemory, compileModule());
}

export async function compileQuoFile(
  stage: string,
  inputFile: string,
  outputFile?: string
) {
  const input = await fs.readFile(inputFile, 'utf8');
  const watOutput = await compileModule(stage, input);
  const watOutputFile = path.format({
    ...path.parse(outputFile || inputFile),
    base: '',
    ext: '.wat',
  });
  await fs.writeFile(watOutputFile, watOutput);
  const wasmModule = (await wabt()).parseWat(watOutputFile, watOutput, {
    exceptions: true,
  });
  const wasmOutputFile =
    outputFile ||
    path.format({
      ...path.parse(inputFile),
      base: '',
      ext: '.wasm',
    });
  await fs.writeFile(wasmOutputFile, wasmModule.toBinary({}).buffer);
  return {watOutputFile, wasmOutputFile};
}

if (require.main === module) {
  (async () => {
    if (process.argv.length < 4) {
      console.error('Usage: quo-driver.js <stage> <input> [output]');
      process.exit(1);
    }
    const stage = process.argv[2];
    const inputFile = process.argv[3];
    const outputFile = process.argv[4];
    console.log(`Compiling ${inputFile} with stage ${stage}`);
    const {watOutputFile, wasmOutputFile} = await compileQuoFile(
      stage,
      inputFile,
      outputFile
    );
    console.log(`-> ${watOutputFile}`);
    console.log(`-> ${wasmOutputFile}`);
  })();
}
