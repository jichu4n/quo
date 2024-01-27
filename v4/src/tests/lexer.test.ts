import {getWasmString, setWasmString, setupWasmModule} from '../quo-driver';

const stages = ['0'];

export interface Token {
  type: number;
  value?: number | string;
}

export async function tokenize(
  stage: string,
  input: string
): Promise<Array<Token>> {
  const {wasmModule, wasmMemory, fns} = await setupWasmModule(stage);
  const {init, nextToken} = fns;

  setWasmString(wasmMemory, 0, input);
  const tokenValuePtr = 4096;
  const tokens: Array<Token> = [];
  init(0);
  do {
    const tokenType = nextToken(tokenValuePtr);
    const token = {
      type: tokenType,
      ...(tokenType === 0
        ? {}
        : {value: getWasmString(wasmMemory, tokenValuePtr)}),
    };
    tokens.push(token);
  } while (tokens[tokens.length - 1].type !== 0);
  return tokens;
}

for (const stage of stages) {
  const testTokenize = async (testCases: Array<[string, Array<Token>]>) => {
    for (const [input, expectedTokens] of testCases) {
      expect(await tokenize(stage, input)).toStrictEqual(expectedTokens);
    }
  };

  describe(`stage ${stage} lexer`, () => {
    test('whitespace', async () => {
      await testTokenize([
        ['\t  \n', [{type: 0}]],
        [
          '  2\r\n  // This is a comment\r\n  3\r\n',
          [{type: 1, value: '2'}, {type: 1, value: '3'}, {type: 0}],
        ],
      ]);
    });

    test('literals', async () => {
      await testTokenize([
        [
          '"hello" 1234 "" -2',
          [
            {type: 2, value: 'hello'},
            {type: 1, value: '1234'},
            {type: 2, value: ''},
            {type: 1, value: '-2'},
            {type: 0},
          ],
        ],
      ]);
      await expect(
        async () => await tokenize(stage, '"hello')
      ).rejects.toThrow();
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
        [
          '{ a = (a + b) * c - d / e; a == b && c != d || e >= f && g < h }',
          [
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
          ],
        ],
      ]);
    });

    test('token length too long', async () => {
      await expect(
        async () => await tokenize(stage, 'a'.repeat(256))
      ).rejects.toThrow();
    });
  });
}
