import fs from 'fs-extra';
import path from 'path';
import {
  compileFiles,
  getRawStr,
  loadQuoWasmModule,
  setRawStr,
} from '../quo-driver';
import {expectUsedChunks} from './memory.test';

const stages = ['0', '1a', '1b', '1c'];

const heapStart = 4096;
const heapEnd = 15 * 1024 * 1024;

async function compileModule(stage: string, input: string): Promise<string> {
  const {wasmMemory, fns} = await loadQuoWasmModule(stage);
  const {init, compileModule, cleanUp} = fns;
  setRawStr(wasmMemory, 0, input);
  init(0, heapStart, heapEnd);
  const result = getRawStr(wasmMemory, compileModule());
  if (stage !== '0') {
    cleanUp();
    expectUsedChunks(wasmMemory, heapStart, 2); // returned string + data chunk
  }
  return result;
}

function getTestInputFiles() {
  const testDir = path.join(__dirname, '..', '..', 'src', 'tests', 'testdata');
  return fs
    .readdirSync(testDir)
    .filter((file) => file.endsWith('.quo'))
    .map((file) => path.join(testDir, file));
}

for (const stage of stages) {
  describe(`stage ${stage} compile module`, () => {
    const testCompileModule = (testCases: Array<readonly [string, string]>) => {
      for (const [input, expectedOutput] of testCases) {
        test(`compile '${input}'`, async () => {
          expect(await compileModule(stage, input)).toStrictEqual(
            expectedOutput
          );
        });
      }
    };

    describe('compileModule', () => {
      testCompileModule([
        ['// empty module', ''],
        ['let x;', '  (global $x (mut i32) (i32.const 0))\n'],
        [
          'let x, y;',
          '  (global $x (mut i32) (i32.const 0)) (global $y (mut i32) (i32.const 0))\n',
        ],
        [
          'fn f1() {} fn f2() {}',
          '  (func $f1 (result i32)\n    (i32.const 0)\n  )\n  (export "f1" (func $f1))\n' +
            '  (func $f2 (result i32)\n    (i32.const 0)\n  )\n  (export "f2" (func $f2))\n',
        ],
      ]);
    });

    for (const inputFile of getTestInputFiles()) {
      test(`compile ${inputFile}`, async () => {
        const outputFile = path.format({
          ...path.parse(inputFile),
          base: '',
          ext: `.stage${stage}.wasm`,
        });
        await compileFiles(stage, [inputFile], outputFile);
      });
    }
  });
}
