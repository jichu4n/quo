import path from 'path';
import {
  compileFilesWithRuntime,
  getRawStr,
  loadQuoWasmModule,
  setRawStr,
} from '../quo-driver';
import {expectUsedMemChunks} from './test-helpers';

interface TestInputFile {
  name: string;
  stages: Array<string>;
}
const testInputFiles: Array<TestInputFile> = [
  {name: 'test1.quo', stages: ['0', '1a', '1b', '1c']},
  {name: 'test-class.quo', stages: ['2a']},
];

const stages = ['0', '1a', '1b', '1c', '2a'];

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
    expectUsedMemChunks(wasmMemory, heapStart, 2); // returned string + data chunk
  }
  return result;
}

function getTestInputFilePath(name: string) {
  return path.join(
    __dirname,
    '..',
    '..',
    'src',
    'tests',
    'testdata',
    `${name}`
  );
}

describe.each(stages)(`stage %s compile module`, (stage) => {
  const testCompileModule = (testCases: Array<readonly [string, string]>) => {
    for (const [input, expectedOutput] of testCases) {
      test(`compile '${input}'`, async () => {
        expect(await compileModule(stage, input)).toStrictEqual(expectedOutput);
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

  test.each(testInputFiles.filter(({stages}) => stages.includes(stage)))(
    'compile test file $name',
    async ({name}) => {
      const inputFilePath = getTestInputFilePath(name);
      const outputFile = path.format({
        ...path.parse(inputFilePath),
        base: '',
        ext: `.stage${stage}.wasm`,
      });
      await compileFilesWithRuntime(stage, [inputFilePath], outputFile);
    }
  );
});
