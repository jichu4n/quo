import {compileStmt, compileBlock} from '../quo0';

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

describe('quo0-compile-stmt-block', () => {
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
  test('block', async () => {
    await testCompileBlock([
      [
        '{ let x; x + 1; }',
        '(local $x i32)\n(i32.add (local.get $x) (i32.const 1))\n',
      ],
    ]);
  });
});
