import {loadQuoWasmModule, setRawStr} from '../quo-driver';
import {expectUsedMemChunks, getStr} from './test-helpers';

const stages = ['2a'];

const heapStart = 4096;
const heapEnd = 15 * 1024 * 1024;

async function compileClass(stage: string, input: string): Promise<string> {
  const {wasmMemory, fns} = await loadQuoWasmModule(stage);
  const {init, compileClass, cleanUp, strFlatten} = fns;
  setRawStr(wasmMemory, 0, input);
  init(0, heapStart, heapEnd);
  const resultAddress = compileClass();
  strFlatten(resultAddress);
  const result = getStr(wasmMemory, resultAddress);
  cleanUp();
  expectUsedMemChunks(wasmMemory, heapStart, 2); // returned string + data chunk
  return result;
}

for (const stage of stages) {
  const testCompileClass = (testCases: Array<readonly [string, string]>) => {
    for (const [input, expectedOutput] of testCases) {
      test(`compile '${input}'`, async () => {
        expect(await compileClass(stage, input)).toStrictEqual(expectedOutput);
      });
    }
  };

  describe(`stage ${stage} compile class`, () => {
    testCompileClass([
      ['class A {}', '  (type $A (struct (field i32)))\n'],
      [
        'class A { x; y; }',
        '  (type $A (struct (field i32) (field $x (mut i32)) (field $y (mut i32))))\n',
      ],
      [
        'class A { x: X; y: Y; }',
        '  (type $A (struct (field i32) (field $x (mut (ref $X))) (field $y (mut (ref $Y)))))\n',
      ],
    ]);
  });
}
