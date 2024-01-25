import {compileStmt, compileBlock, compileFn} from '../quo0';

type TestCase = [string, string | Array<string>];
type TestCases = Array<TestCase>;

function expectedOutputToString(expectedOutput: string | Array<string>) {
  return Array.isArray(expectedOutput)
    ? expectedOutput.join('\n') + '\n'
    : expectedOutput;
}

async function testCompileStmt(testCases: TestCases) {
  for (const [input, expectedOutput] of testCases) {
    expect(await compileStmt(input)).toStrictEqual(
      expectedOutputToString(expectedOutput)
    );
  }
}

async function testCompileBlock(testCases: TestCases) {
  for (const [input, expectedOutput] of testCases) {
    expect(await compileBlock(input)).toStrictEqual(
      expectedOutputToString(expectedOutput)
    );
  }
}

async function testCompileFn(testCases: TestCases) {
  for (const [input, expectedOutput] of testCases) {
    expect(await compileFn(input)).toStrictEqual(
      expectedOutputToString(expectedOutput)
    );
  }
}

describe('quo0-compile-stmt-block-fn', () => {
  test('expr', async () => {
    await testCompileStmt([['42;', '(drop (i32.const 42))']]);
  });
  test('let', async () => {
    await testCompileStmt([
      ['let x;', '(local $x i32)'],
      ['let x, y;', '(local $x i32) (local $y i32)'],
      ['let x, y, z;', '(local $x i32) (local $y i32) (local $z i32)'],
    ]);
  });
  test('if', async () => {
    await testCompileStmt([
      [
        'if (1) {}',
        `
(if (i32.const 1)
  (then
  )
)`.trim(),
      ],
      [
        'if (1 > 0) { foo(); }',
        `
(if (i32.gt_s (i32.const 1) (i32.const 0))
  (then
    (drop (call $foo))
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
  test('while', async () => {
    await testCompileStmt([
      [
        'while (1) {}',
        `
(block $loop_1_end (loop $loop_1
  (br_if $loop_1_end (i32.const 1))
  (br $loop_1)
))`.trim(),
      ],
      [
        'while (1) { foo(); }',
        `
(block $loop_1_end (loop $loop_1
  (br_if $loop_1_end (i32.const 1))
  (drop (call $foo))
  (br $loop_1)
))`.trim(),
      ],
      [
        'while (1) { foo(); break; }',
        `
(block $loop_1_end (loop $loop_1
  (br_if $loop_1_end (i32.const 1))
  (drop (call $foo))
  (br $loop_1_end)
  (br $loop_1)
))`.trim(),
      ],
      [
        'while (1) { foo(); while (2) { if (3) continue; break; } }',
        `
(block $loop_1_end (loop $loop_1
  (br_if $loop_1_end (i32.const 1))
  (drop (call $foo))
  (block $loop_2_end (loop $loop_2
    (br_if $loop_2_end (i32.const 2))
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
  test('return', async () => {
    await testCompileStmt([
      ['return;', '(return)'],
      ['return 0;', '(return (i32.const 0))'],
      ['return 10 * 5;', '(return (i32.mul (i32.const 10) (i32.const 5)))'],
    ]);
  });
  test('block', async () => {
    await testCompileBlock([
      ['{ }', ''],
      ['{ f(); }', '(drop (call $f))\n'],
      [
        '{ let x; x = x + 1; }',
        '(local $x i32)\n(local.set $x (i32.add (local.get $x) (i32.const 1)))\n',
      ],
    ]);
  });
  test('fn', async () => {
    await testCompileFn([
      [
        'fn foo() {}',
        `
  (func $foo
  )
  (export "foo" (func $foo))
`.slice(1),
      ],
      [
        'fn foo(x) {}',
        `
  (func $foo (param $x i32)
  )
  (export "foo" (func $foo))
`.slice(1),
      ],
      [
        'fn foo(x, y) {}',
        `
  (func $foo (param $x i32) (param $y i32)
  )
  (export "foo" (func $foo))
`.slice(1),
      ],
      [
        'fn foo(): Int { return 42; }',
        `
  (func $foo (result i32)
    (return (i32.const 42))
  )
  (export "foo" (func $foo))
`.slice(1),
      ],
      [
        'fn foo(x): Int { return x; }',
        `
  (func $foo (param $x i32) (result i32)
    (return (local.get $x))
  )
  (export "foo" (func $foo))
`.slice(1),
      ],
    ]);
  });
});
