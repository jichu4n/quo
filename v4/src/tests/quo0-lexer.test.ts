import {Token, tokenize} from '../quo0';

async function testTokenize(testCases: Array<[string, Array<Token>]>) {
  for (const [input, expectedTokens] of testCases) {
    expect(await tokenize(input)).toStrictEqual(expectedTokens);
  }
}

describe('quo0-lexer', () => {
  test('whitespace', async () => {
    await testTokenize([
      ['\t  \n', [{type: 0}]],
      [
        '  2\r\n  // This is a comment\r\n  3\r\n',
        [{type: 1, value: 2}, {type: 1, value: 3}, {type: 0}],
      ],
    ]);
  });

  test('literals', async () => {
    await testTokenize([
      [
        '"hello" 1234 ""',
        [
          {type: 2, value: 'hello'},
          {type: 1, value: 1234},
          {type: 2, value: ''},
          {type: 0},
        ],
      ],
    ]);
    await expect(async () => await tokenize('"hello')).rejects.toThrow();
  });

  test('identifiers and keywords', async () => {
    await testTokenize([
      [
        'aAa _b_1 C$13 sif ifs fn let if else while return',
        [
          {type: 3, value: 'aAa'},
          {type: 3, value: '_b_1'},
          {type: 3, value: 'C$13'},
          {type: 3, value: 'sif'},
          {type: 3, value: 'ifs'},
          {type: 4, value: 'fn'},
          {type: 5, value: 'let'},
          {type: 6, value: 'if'},
          {type: 7, value: 'else'},
          {type: 8, value: 'while'},
          {type: 9, value: 'return'},
          {type: 0},
        ],
      ],
    ]);
  });

  test('operators', async () => {
    await testTokenize([
      ['{ a = (a + b) * c - d / e; a == b && c != d || e >= f && g < h }', [
        {type: 123, value: '{'},
        {type: 3, value: 'a'},
        {type: 61, value: '='},
        {type: 40, value: '('},
        {type: 3, value: 'a'},
        {type: 43, value: '+'},
        {type: 3, value: 'b'},
        {type: 41, value: ')'},
        {type: 42, value: '*'},
        {type: 3, value: 'c'},
        {type: 45, value: '-'},
        {type: 3, value: 'd'},
        {type: 47, value: '/'},
        {type: 3, value: 'e'},
        {type: 59, value: ';'},
        {type: 3, value: 'a'},
        {type: 61 + 128, value: '=='},
        {type: 3, value: 'b'},
        {type: 38 + 128, value: '&&'},
        {type: 3, value: 'c'},
        {type: 33 + 128, value: '!='},
        {type: 3, value: 'd'},
        {type: 124 + 128, value: '||'},
        {type: 3, value: 'e'},
        {type: 62 + 128, value: '>='},
        {type: 3, value: 'f'},
        {type: 38 + 128, value: '&&'},
        {type: 3, value: 'g'},
        {type: 60, value: '<'},
        {type: 3, value: 'h'},
        {type: 125, value: '}'},
        {type: 0},
      ]],
    ]);
  });
});
