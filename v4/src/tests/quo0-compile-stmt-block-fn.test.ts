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
      ['if (1) {}', '(if (i32.const 1)\n  (then\n  )\n)'],
      [
        'if (1 > 0) { foo(); }',
        '(if (i32.gt_s (i32.const 1) (i32.const 0))\n  (then\n    (drop (call $foo))\n  )\n)',
      ],
      [
        'if (1 > 0) { foo(); } else {}',
        '(if (i32.gt_s (i32.const 1) (i32.const 0))\n  (then\n    (drop (call $foo))\n  )\n  (else\n  )\n)',
      ],
      [
        'if (1 > 0) { foo(); } else { bar(); }',
        '(if (i32.gt_s (i32.const 1) (i32.const 0))\n  (then\n    (drop (call $foo))\n  )\n  (else\n    (drop (call $bar))\n  )\n)',
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
      ['fn foo() {}', '  (func $foo\n  )\n  (export "foo" (func $foo))\n'],
      [
        'fn foo(x) {}',
        '  (func $foo (param $x i32)\n  )\n  (export "foo" (func $foo))\n',
      ],
      [
        'fn foo(x, y) {}',
        '  (func $foo (param $x i32) (param $y i32)\n  )\n  (export "foo" (func $foo))\n',
      ],
      [
        'fn foo(x, y, z) {}',
        '  (func $foo (param $x i32) (param $y i32) (param $z i32)\n  )\n  (export "foo" (func $foo))\n',
      ],
      [
        'fn foo(): Int { return 42; }',
        '  (func $foo (result i32)\n    (return (i32.const 42))\n  )\n  (export "foo" (func $foo))\n',
      ],
      [
        'fn foo(x): Int { return x; }',
        '  (func $foo (param $x i32) (result i32)\n    (return (local.get $x))\n  )\n  (export "foo" (func $foo))\n',
      ],
    ]);
  });
});
