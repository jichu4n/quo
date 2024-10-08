import fs from 'fs-extra';
import path from 'path';
import wabt from 'wabt';
import {program} from 'commander';
import {execFile} from 'child_process';

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
  return await loadWasmModule(
    path.join(__dirname, '..', 'dist', `quo${stage}.wasm`)
  );
}

export async function loadWasmModule(wasmFilePath: string) {
  const wasmFile = await fs.readFile(wasmFilePath);
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

export enum Assembler {
  WABT = 'wabt',
  BINARYEN = 'binaryen',
}

type AssemblerFn = (args: {
  watFilePath: string;
  watModule: string;
  wasmOutputFilePath: string;
}) => Promise<void>;

export const AssemberMap: {[key in Assembler]: AssemblerFn} = {
  [Assembler.WABT]: assembleWatModuleWithWabt,
  [Assembler.BINARYEN]: assembleWatModuleWithBinaryen,
};

async function assembleWatModuleWithWabt({
  watFilePath,
  watModule,
  wasmOutputFilePath,
}: {
  watFilePath: string;
  watModule: string;
  wasmOutputFilePath: string;
}) {
  const wasmModule = (await wabt()).parseWat(watFilePath, watModule, {
    exceptions: true,
    gc: true,
    reference_types: true,
    function_references: true,
  });
  const {buffer} = wasmModule.toBinary({});
  await fs.writeFile(wasmOutputFilePath, buffer);
}

async function assembleWatModuleWithBinaryen({
  watFilePath,
  wasmOutputFilePath,
}: {
  watFilePath: string;
  watModule: string;
  wasmOutputFilePath: string;
}) {
  const wasmAsPath = path.join(
    __dirname,
    '..',
    'node_modules',
    '.bin',
    'wasm-as'
  );
  const args = ['--all-features', '-o', wasmOutputFilePath, watFilePath];
  await new Promise<void>((resolve, reject) => {
    const child = execFile(wasmAsPath, args, (err) =>
      err ? reject(err) : resolve()
    );
    child.stderr?.pipe(process.stderr);
    child.stdout?.pipe(process.stdout);
  });
}

export async function compileFiles(
  stage: string,
  inputFiles: Array<string>,
  outputFile?: string,
  {
    stringConstantsSize = defaultStringConstantsSize,
    heapEnd = defaultHeapEnd,
    assembler = Assembler.BINARYEN,
  }: {
    stringConstantsSize?: number;
    heapEnd?: number;
    assembler?: Assembler;
  } = {}
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
${inputFiles.map((inputFile) => `;;   - ${inputFile}`).join('\n')}

(module
${watOutputs.join('\n')}
)
`.trimStart();
  const watOutputFilePath = path.format({
    ...path.parse(outputFile || inputFiles[inputFiles.length - 1]),
    base: '',
    ext: '.wat',
  });
  await fs.writeFile(watOutputFilePath, watModule);
  const wasmOutputFilePath =
    outputFile ||
    path.format({
      ...path.parse(inputFiles[inputFiles.length - 1]),
      base: '',
      ext: '.wasm',
    });
  const assemblerFn = AssemberMap[assembler];
  await assemblerFn({
    watFilePath: watOutputFilePath,
    watModule,
    wasmOutputFilePath,
  });
  return {watOutputFilePath, wasmOutputFilePath};
}

function getRuntimeFilePath(name: string) {
  return path.join(__dirname, '..', 'dist', name);
}

function getRuntimeFilesForStage(stage: string) {
  const runtimeFiles = ['quo0-runtime.wat'];
  if (stage[0] > '0') {
    runtimeFiles.push('quo1-runtime.quo');
  }
  if (stage[0] > '1') {
    runtimeFiles.push('quo2-runtime.quo');
  }
  return runtimeFiles;
}

function getRuntimeFilePathsForStage(stage: string) {
  return getRuntimeFilesForStage(stage).map(getRuntimeFilePath);
}

export async function compileFilesWithRuntime(
  ...args: Parameters<typeof compileFiles>
) {
  const [stage, inputFiles, ...rest] = args;
  return await compileFiles(
    stage,
    [...getRuntimeFilePathsForStage(stage), ...inputFiles],
    ...rest
  );
}

if (require.main === module) {
  program
    .name('quo')
    .description('Quo compiler')
    .argument('<stage>', 'compiler stage, e.g. 0, 1a, 1b')
    .argument('<input-files...>', 'Quo and WAT source files')
    .option('-o, --output <output>', 'output WebAssembly (.wasm) file')
    .option('--no-rt', 'do not include runtime files')
    .option(
      '--assembler <as>',
      'assembler to use ("wabt" or "binaryen")',
      'binaryen'
    )
    .action(async (stage, inputFiles) => {
      const {output: outputFile, noRt, assembler} = program.opts();
      const compileFn = noRt ? compileFiles : compileFilesWithRuntime;
      if (!Object.values(Assembler).includes(assembler)) {
        throw new Error(`Unknown assembler "${assembler}"`);
      }
      const {watOutputFilePath, wasmOutputFilePath} = await compileFn(
        stage,
        inputFiles,
        outputFile,
        {assembler}
      );
      console.log(`-> ${watOutputFilePath}`);
      console.log(`-> ${wasmOutputFilePath}`);
    });
  program.parse(process.argv);
}
