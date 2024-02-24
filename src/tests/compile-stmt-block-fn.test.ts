import {getRawStr, loadQuoWasmModule, setRawStr} from '../quo-driver';
import {expectUsedMemChunks, getStr} from './test-helpers';

const stages = ['0', '1a', '1b', '1c', '2a'];

const heapStart = 4096;
const heapEnd = 15 * 1024 * 1024;

async function compile(
  fnName: string,
  fnArgs: Array<unknown>,
  stage: string,
  input: string
): Promise<string> {
  const {wasmMemory, fns} = await loadQuoWasmModule(stage);
  const {init, strFlatten, cleanUp} = fns;
  const fn = fns[fnName];
  setRawStr(wasmMemory, 0, input);
  init(0, heapStart, heapEnd);
  const resultAddress = fn(...fnArgs);
  let result: string;
  if (stage === '0') {
    result = getRawStr(wasmMemory, resultAddress);
  } else {
    strFlatten(resultAddress);
    result = getStr(wasmMemory, resultAddress);
    cleanUp();
    expectUsedMemChunks(wasmMemory, heapStart, 2); // returned string + data chunk
  }
  return result;
}

type TestCase = [string, string | Array<string>];
type TestCases = Array<TestCase>;

function expectedOutputToString(expectedOutput: string | Array<string>) {
  return Array.isArray(expectedOutput)
    ? expectedOutput.join('\n') + '\n'
    : expectedOutput;
}

for (const stage of stages) {
  const compileStmt = compile.bind(null, 'compileStmt', [0], stage);
  const compileBlock = compile.bind(null, 'compileBlock', [0], stage);
  const compileFn = compile.bind(null, 'compileFn', [], stage);
  const testCompile = (
    compile: (input: string) => Promise<string>,
    testCases: TestCases
  ) => {
    for (const [input, expectedOutput] of testCases) {
      test(`compile '${input}'`, async () => {
        expect(await compile(input)).toStrictEqual(
          expectedOutputToString(expectedOutput)
        );
      });
    }
  };
  const testCompileStmt = testCompile.bind(null, compileStmt);
  const testCompileBlock = testCompile.bind(null, compileBlock);
  const testCompileFn = testCompile.bind(null, compileFn);

  describe(`stage ${stage} compile stmt, block, & fn`, () => {
    describe('expr', () => {
      testCompileStmt([['42;', '(drop (i32.const 42))']]);
    });
    describe('let', () => {
      testCompileStmt([
        ['let x;', '(local $x i32)'],
        ['let x, y;', '(local $x i32) (local $y i32)'],
        ['let x, y, z;', '(local $x i32) (local $y i32) (local $z i32)'],
        ...(stage[0] === '2'
          ? ([
              ['let x: Foo;', '(local $x i32)'],
              [
                'let x: Foo, y: Bar;',
                '(local $x i32) (local $y i32)',
              ],
            ] as TestCases)
          : []),
      ]);
    });
    describe('if', () => {
      testCompileStmt([
        [
          'if (1) {}',
          `
(if (i32.const 1)
  (then
  )
)`.trim(),
        ],
        [
          'if (1 > 0) {}',
          `
(if (i32.gt_s (i32.const 1) (i32.const 0))
  (then
  )
)`.trim(),
        ],
        [
          'if (1 > 0) { 3; }',
          `
(if (i32.gt_s (i32.const 1) (i32.const 0))
  (then
    (drop (i32.const 3))
  )
)`.trim(),
        ],
        [
          'if (1 > 0) { foo(); } else {}',
          `
(if (i32.gt_s (i32.const 1) (i32.const 0))
  (then
    (drop (call $foo))
  )
  (else
  )
)`.trim(),
        ],
        [
          'if (1 > 0) { foo(); } else { bar(); }',
          `
(if (i32.gt_s (i32.const 1) (i32.const 0))
  (then
    (drop (call $foo))
  )
  (else
    (drop (call $bar))
  )
)`.trim(),
        ],
      ]);
    });
    describe('while', () => {
      testCompileStmt([
        [
          'while (1) {}',
          `
(block $loop_1_end (loop $loop_1
  (br_if $loop_1_end (i32.eqz (i32.const 1)))
  (br $loop_1)
))`.trim(),
        ],
        [
          'while (1) { foo(); }',
          `
(block $loop_1_end (loop $loop_1
  (br_if $loop_1_end (i32.eqz (i32.const 1)))
  (drop (call $foo))
  (br $loop_1)
))`.trim(),
        ],
        [
          'while (1) { foo(); break; }',
          `
(block $loop_1_end (loop $loop_1
  (br_if $loop_1_end (i32.eqz (i32.const 1)))
  (drop (call $foo))
  (br $loop_1_end)
  (br $loop_1)
))`.trim(),
        ],
        [
          'while (1) { foo(); while (2) { if (3) continue; break; } }',
          `
(block $loop_1_end (loop $loop_1
  (br_if $loop_1_end (i32.eqz (i32.const 1)))
  (drop (call $foo))
  (block $loop_2_end (loop $loop_2
    (br_if $loop_2_end (i32.eqz (i32.const 2)))
    (if (i32.const 3)
      (then
        (br $loop_2)
      )
    )
    (br $loop_2_end)
    (br $loop_2)
  ))
  (br $loop_1)
))`.trim(),
        ],
      ]);
    });
    describe('return', () => {
      testCompileStmt([
        ['return;', '(return (i32.const 0))'],
        ['return 0;', '(return (i32.const 0))'],
        ['return 10 * 5;', '(return (i32.mul (i32.const 10) (i32.const 5)))'],
      ]);
    });
    describe('block', () => {
      testCompileBlock([
        ['{ }', ''],
        ['{ f(); }', '(drop (call $f))\n'],
        [
          '{ let x; x = x + 1; f(x); }',
          `
(local $x i32)
(local.set $x (i32.add (local.get $x) (i32.const 1)))
(drop (call $f (local.get $x)))
`.trimStart(),
        ],
      ]);
    });
    describe('fn', () => {
      testCompileFn([
        [
          'fn foo() {}',
          `
  (func $foo (result i32)
    (i32.const 0)
  )
  (export "foo" (func $foo))
`.slice(1),
        ],
        [
          'fn foo(x) {}',
          `
  (func $foo (param $x i32) (result i32)
    (i32.const 0)
  )
  (export "foo" (func $foo))
`.slice(1),
        ],
        [
          'fn foo(x, y) {}',
          `
  (func $foo (param $x i32) (param $y i32) (result i32)
    (i32.const 0)
  )
  (export "foo" (func $foo))
`.slice(1),
        ],
        [
          'fn foo() { return 42; }',
          `
  (func $foo (result i32)
    (return (i32.const 42))
    (i32.const 0)
  )
  (export "foo" (func $foo))
`.slice(1),
        ],
        [
          'fn foo(x) { return x; }',
          `
  (func $foo (param $x i32) (result i32)
    (return (local.get $x))
    (i32.const 0)
  )
  (export "foo" (func $foo))
`.slice(1),
        ],
        ...(stage[0] === '2'
          ? ([
              [
                'fn foo(x: Foo) {}',
                `
  (func $foo (param $x i32) (result i32)
    (i32.const 0)
  )
  (export "foo" (func $foo))
`.slice(1),
              ],
              [
                'fn foo(x: Foo): Bar {}',
                `
  (func $foo (param $x i32) (result i32)
    (i32.const 0)
  )
  (export "foo" (func $foo))
`.slice(1),
              ],
            ] as TestCases)
          : []),
      ]);
    });
  });
}
