import fs from 'fs-extra';
import path from 'path';
import wabt from 'wabt';
import {program} from 'commander';

const defaultMemoryPages = 256; // 256 * 64KB = 16MB
export const defaultHeapEnd = 15 * 1024 * 1024;
// Size of all string literals per source file.
const defaultStringConstantsSize = 32 * 1024;

export function getRawStr(memory: WebAssembly.Memory, ptr: number) {
  const bytes = new Uint8Array(memory.buffer, ptr);
  const len = bytes.findIndex((byte) => byte === 0);
  return new TextDecoder().decode(new Uint8Array(memory.buffer, ptr, len));
}

export function setRawStr(
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

export async function loadQuoWasmModule(stage: string) {
  const wasmFile = await fs.readFile(
    path.join(__dirname, '..', 'dist', `quo${stage}.wasm`)
  );
  const wasmMemory = new WebAssembly.Memory({initial: defaultMemoryPages});
  const wasmModule = await WebAssembly.instantiate(wasmFile, {
    env: {memory: wasmMemory, tag: wasmExceptionTag},
    debug: {
      puts(ptr: number): number {
        console.log(getRawStr(wasmMemory, ptr));
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

export class WebAssemblyError extends Error {
  constructor(wasmMemory: WebAssembly.Memory, e: unknown) {
    if (e instanceof WebAssembly.Exception && e.is(wasmExceptionTag)) {
      super(getRawStr(wasmMemory, e.getArg(wasmExceptionTag, 0)));
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
  input: string,
  {heapEnd = defaultHeapEnd}: {heapEnd?: number} = {}
): Promise<string> {
  const {wasmMemory, fns} = await loadQuoWasmModule(stage);
  const {init, compileModule} = fns;
  const len = setRawStr(wasmMemory, 0, input);
  const heapStart = ((len + 3) >> 2) << 2;
  init(0, heapStart, heapEnd);
  return getRawStr(wasmMemory, compileModule());
}

export async function compileFiles(
  stage: string,
  inputFiles: Array<string>,
  outputFile?: string,
  {
    stringConstantsSize = defaultStringConstantsSize,
    heapEnd = defaultHeapEnd,
  }: {stringConstantsSize?: number; heapEnd?: number} = {}
) {
  // Compile input files to WAT.
  const watOutputs: Array<string> = [];
  for (const inputFile of inputFiles) {
    const inputFileExt = path.extname(inputFile).toLowerCase();
    const input = await fs.readFile(inputFile, 'utf8');
    let watOutput: string;
    if (inputFileExt === '.wat') {
      watOutput = input;
    } else if (inputFileExt === '.quo') {
      if (heapEnd + stringConstantsSize > defaultMemoryPages * 64 * 1024) {
        throw new Error('Size of string constants exceeds memory size');
      }
      watOutput = (await compileModule(stage, input, {heapEnd})).trimEnd();
      heapEnd += stringConstantsSize;
    } else {
      throw new Error(`Unknown file extension in input file "${inputFile}"`);
    }
    watOutputs.push(`
  ;; >>>> ${inputFile}
${watOutput}
  ;; <<<< ${inputFile}`);
  }
  const watModule = `
;; Compiled by quo stage ${stage}
(module
${watOutputs.join('\n')}
)
`.trimStart();
  const watOutputFile = path.format({
    ...path.parse(outputFile || inputFiles[0]),
    base: '',
    ext: '.wat',
  });
  await fs.writeFile(watOutputFile, watModule);
  const wasmModule = (await wabt()).parseWat(watOutputFile, watModule, {
    exceptions: true,
    gc: true,
    reference_types: true,
    function_references: true,
  });
  const wasmOutputFile =
    outputFile ||
    path.format({
      ...path.parse(inputFiles[0]),
      base: '',
      ext: '.wasm',
    });
  await fs.writeFile(wasmOutputFile, wasmModule.toBinary({}).buffer);
  return {watOutputFile, wasmOutputFile};
}

if (require.main === module) {
  program
    .name('quo')
    .description('Quo compiler')
    .argument('<stage>', 'compiler stage, e.g. 0, 1a, 1b')
    .argument('<input-files...>', 'Quo and WAT source files')
    .option('-o, --output <output>', 'output WebAssembly (.wasm) file')
    .action(async (stage, inputFiles) => {
      const {output: outputFile} = program.opts();
      const {watOutputFile, wasmOutputFile} = await compileFiles(
        stage,
        inputFiles,
        outputFile
      );
      console.log(`-> ${watOutputFile}`);
      console.log(`-> ${wasmOutputFile}`);
    });
  program.parse(process.argv);
}
