import {compileStmt, compileBlock, compileFn} from '../quo0';

async function testCompileStmt(testCases: Array<[string, string]>) {
  for (const [input, expectedOutput] of testCases) {
    expect(await compileStmt(input)).toStrictEqual(expectedOutput);
  }
}

async function testCompileBlock(testCases: Array<[string, string]>) {
  for (const [input, expectedOutput] of testCases) {
    expect(await compileBlock(input)).toStrictEqual(expectedOutput);
  }
}

async function testCompileFn(testCases: Array<[string, string]>) {
  for (const [input, expectedOutput] of testCases) {
    expect(await compileFn(input)).toStrictEqual(expectedOutput);
  }
}

describe('quo0-compile-stmt-block-fn', () => {
  test('expr', async () => {
    await testCompileStmt([['42;', '(i32.const 42)']]);
  });
  test('let', async () => {
    await testCompileStmt([
      ['let x;', '(local $x i32)'],
      ['let x, y;', '(local $x i32) (local $y i32)'],
      ['let x, y, z;', '(local $x i32) (local $y i32) (local $z i32)'],
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
      ['{ f(); }', '(call $f)\n'],
      [
        '{ let x; x = x + 1; }',
        '(local $x i32)\n(local.set $x (i32.add (local.get $x) (i32.const 1)))\n',
      ],
    ]);
  });
  test('fn', async () => {
    await testCompileFn([
      ['fn foo() {}', '  (func $foo\n  )\n'],
      ['fn foo(x) {}', '  (func $foo (param $x i32)\n  )\n'],
      ['fn foo(x, y) {}', '  (func $foo (param $x i32) (param $y i32)\n  )\n'],
      [
        'fn foo(x, y, z) {}',
        '  (func $foo (param $x i32) (param $y i32) (param $z i32)\n  )\n',
      ],
      [
        'fn foo(): Int { return 42; }',
        '  (func $foo (result i32)\n    (return (i32.const 42))\n  )\n',
      ],
      [
        'fn foo(x): Int { return x; }',
        '  (func $foo (param $x i32) (result i32)\n    (return (local.get $x))\n  )\n',
      ],
    ]);
  });
});
