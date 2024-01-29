import {
  defaultHeapEnd,
  getWasmString,
  loadQuoWasmModule,
  setWasmString,
} from '../quo-driver';
import {expectUsedChunks} from './memory.test';
import {getWasmStr} from './strings.test';

const stages = ['0', '1a'];

const heapStart = 4096;
const heapEnd = 15 * 1024 * 1024;

async function compileExpr(stage: string, input: string): Promise<string> {
  const {wasmMemory, fns} = await loadQuoWasmModule(stage);
  const {init, compileExpr, cleanUp, strFlatten} = fns;
  setWasmString(wasmMemory, 0, input);
  init(0, heapStart, heapEnd);
  const resultAddress = compileExpr();
  let result: string;
  if (stage === '0') {
    result = getWasmString(wasmMemory, resultAddress);
  } else {
    strFlatten(resultAddress);
    result = getWasmStr(wasmMemory, resultAddress);
    cleanUp();
    expectUsedChunks(wasmMemory, heapStart, 2); // returned string + data chunk
  }
  return result;
}

for (const stage of stages) {
  const testCompileExpr = (testCases: Array<readonly [string, string]>) => {
    for (const [input, expectedOutput] of testCases) {
      test(`compile '${input}'`, async () => {
        expect(await compileExpr(stage, input)).toStrictEqual(expectedOutput);
      });
    }
  };

  describe(`stage ${stage} compile expr`, () => {
    describe('compileExpr0', () => {
      testCompileExpr([
        ['-100', '(i32.const -100)'],
        ['42', '(i32.const 42)'],
        ['(32)', '(i32.const 32)'],
        ['"hello"', `(i32.const ${defaultHeapEnd})`],
        ['hello()', '(call $hello)'],
        ['hello(1)', '(call $hello (i32.const 1))'],
        ['hello(1, 2)', '(call $hello (i32.const 1) (i32.const 2))'],
        [
          'hello(1, 2, 3)',
          '(call $hello (i32.const 1) (i32.const 2) (i32.const 3))',
        ],
        ['(f(g()))', '(call $f (call $g))'],
      ]);
      test('invalid input', async () => {
        await expect(compileExpr(stage, ';')).rejects.toThrow();
        await expect(compileExpr(stage, '(3')).rejects.toThrow();
      });
    });

    describe('compileExpr1', () => {
      testCompileExpr([
        ['-(3)', '(i32.neg (i32.const 3))'],
        ['!0', '(i32.eqz (i32.const 0))'],
      ]);
    });
    describe('compileExpr2', () => {
      testCompileExpr([
        ['1 * 2', '(i32.mul (i32.const 1) (i32.const 2))'],
        [
          '1 / 2 * 3 / 4',
          '(i32.div_s (i32.mul (i32.div_s (i32.const 1) (i32.const 2)) (i32.const 3)) (i32.const 4))',
        ],
        [
          '1 / (2 * (3 % 4))',
          '(i32.div_s (i32.const 1) (i32.mul (i32.const 2) (i32.rem_s (i32.const 3) (i32.const 4))))',
        ],
      ]);
    });
    describe('compileExpr3', () => {
      testCompileExpr([
        ['1 + 2', '(i32.add (i32.const 1) (i32.const 2))'],
        [
          '1 - 2 * 3 + 4',
          '(i32.add (i32.sub (i32.const 1) (i32.mul (i32.const 2) (i32.const 3))) (i32.const 4))',
        ],
        [
          '(1 - 2) * (3 + 4)',
          '(i32.mul (i32.sub (i32.const 1) (i32.const 2)) (i32.add (i32.const 3) (i32.const 4)))',
        ],
      ]);
    });
    describe('compileExpr4', () => {
      testCompileExpr([
        ['1 == 2', '(i32.eq (i32.const 1) (i32.const 2))'],
        [
          '1 + 2 != 2 * 2',
          '(i32.ne (i32.add (i32.const 1) (i32.const 2)) (i32.mul (i32.const 2) (i32.const 2)))',
        ],
        [
          '(1 >= 2) > (0 <= 1)',
          '(i32.gt_s (i32.ge_s (i32.const 1) (i32.const 2)) (i32.le_s (i32.const 0) (i32.const 1)))',
        ],
      ]);
    });
    describe('compileExpr5 and compileExpr6', () => {
      testCompileExpr([
        [
          '1 == 2 || 3 == 4',
          '(call $or (i32.eq (i32.const 1) (i32.const 2)) (i32.eq (i32.const 3) (i32.const 4)))',
        ],
        [
          '1 == 2 || 3 == 4 && 5 == 6',
          '(call $or (i32.eq (i32.const 1) (i32.const 2)) (call $and (i32.eq (i32.const 3) (i32.const 4)) (i32.eq (i32.const 5) (i32.const 6))))',
        ],
        [
          '1 || 3 == 4 && 5 == 6 || 7',
          '(call $or (call $or (i32.const 1) (call $and (i32.eq (i32.const 3) (i32.const 4)) (i32.eq (i32.const 5) (i32.const 6)))) (i32.const 7))',
        ],
        [
          '1 == 2 || 3 == 4 && 5 == 6 || 7 == 8',
          '(call $or (call $or (i32.eq (i32.const 1) (i32.const 2)) (call $and (i32.eq (i32.const 3) (i32.const 4)) (i32.eq (i32.const 5) (i32.const 6)))) (i32.eq (i32.const 7) (i32.const 8)))',
        ],
        [
          '1 == 2 || 3 == 4 && 5 == 6 || 7 == 8 || 9 == 10',
          '(call $or (call $or (call $or (i32.eq (i32.const 1) (i32.const 2)) (call $and (i32.eq (i32.const 3) (i32.const 4)) (i32.eq (i32.const 5) (i32.const 6)))) (i32.eq (i32.const 7) (i32.const 8))) (i32.eq (i32.const 9) (i32.const 10)))',
        ],
        [
          '!(1 == 2 && 3 >= 4)',
          '(i32.eqz (call $and (i32.eq (i32.const 1) (i32.const 2)) (i32.ge_s (i32.const 3) (i32.const 4))))',
        ],
      ]);
    });

    if (stage === '0') {
      test('expression too long', async () => {
        expect(
          (await compileExpr(stage, '1 + '.repeat(10) + '1')).length
        ).toBeGreaterThan(0);
        await expect(
          compileExpr(stage, '1 + '.repeat(100) + '1')
        ).rejects.toThrow();
      });
    }
  });
}
