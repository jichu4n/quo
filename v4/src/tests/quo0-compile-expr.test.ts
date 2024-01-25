import {compileExpr} from '../quo0';

async function testCompileExpr(testCases: Array<[string, string]>) {
  for (const [input, expectedOutput] of testCases) {
    expect(await compileExpr(input)).toStrictEqual(expectedOutput);
  }
}

describe('quo0-compile-expr', () => {
  test('compileExpr0', async () => {
    await testCompileExpr([
      ['-100', '(i32.const -100)'],
      ['42', '(i32.const 42)'],
      ['(32)', '(i32.const 32)'],
      ['hello()', '(call $hello)'],
      ['hello(1)', '(call $hello (i32.const 1))'],
      ['hello(1, 2)', '(call $hello (i32.const 1) (i32.const 2))'],
      [
        'hello(1, 2, 3)',
        '(call $hello (i32.const 1) (i32.const 2) (i32.const 3))',
      ],
      ['(f(g()))', '(call $f (call $g))'],
    ]);
  });
  test('compileExpr1', async () => {
    await testCompileExpr([
      ['-(3)', '(i32.neg (i32.const 3))'],
      ['!0', '(i32.eqz (i32.const 0))'],
    ]);
  });
  test('compileExpr2', async () => {
    await testCompileExpr([
      ['1 * 2', '(i32.mul (i32.const 1) (i32.const 2))'],
      [
        '1 / 2 * 3 / 4',
        '(i32.div_s (i32.mul (i32.div_s (i32.const 1) (i32.const 2)) (i32.const 3)) (i32.const 4))',
      ],
      [
        '1 / (2 * (3 / 4))',
        '(i32.div_s (i32.const 1) (i32.mul (i32.const 2) (i32.div_s (i32.const 3) (i32.const 4))))',
      ],
    ]);
  });
  test('compileExpr3', async () => {
    await testCompileExpr([
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
  test('compileExpr4', async () => {
    await testCompileExpr([
      ['1 == 2', '(i32.eq (i32.const 1) (i32.const 2))'],
      [
        '1 + 2 != 2 * 2',
        '(i32.ne (i32.add (i32.const 1) (i32.const 2)) (i32.mul (i32.const 2) (i32.const 2)))',
      ],
      [
        '(1 == 2) > (0 <= 1)',
        '(i32.gt_s (i32.eq (i32.const 1) (i32.const 2)) (i32.le_s (i32.const 0) (i32.const 1)))',
      ],
    ]);
  });
  test('compileExpr5 and compileExpr6', async () => {
    await testCompileExpr([
      [
        '1 == 2 || 3 == 4 && 5 == 6 || 7 == 8',
        '(i32.or (i32.or (i32.eq (i32.const 1) (i32.const 2)) (i32.and (i32.eq (i32.const 3) (i32.const 4)) (i32.eq (i32.const 5) (i32.const 6)))) (i32.eq (i32.const 7) (i32.const 8)))',
      ],
      [
        '!(1 == 2 && 3 >= 4)',
        '(i32.eqz (i32.and (i32.eq (i32.const 1) (i32.const 2)) (i32.ge_s (i32.const 3) (i32.const 4))))',
      ],
    ]);
  });

  test('expression too long', async () => {
    expect((await compileExpr('1 + '.repeat(10) + '1')).length).toBeGreaterThan(
      0
    );
    await expect(compileExpr('1 + '.repeat(100) + '1')).rejects.toThrow();
  });
});
