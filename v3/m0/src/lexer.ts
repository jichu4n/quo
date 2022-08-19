import moo, {Token} from 'moo';

export enum Keywords {
  CLASS = 'class',
  ELSE = 'else',
  FN = 'fn',
  IF = 'if',
  IMPORT = 'import',
  LET = 'let',
  NEW = 'new',
  NULL = 'null',
  RETURN = 'return',
  WHILE = 'while',
}

/** moo lexer used internally by Lexer. */
const mooLexer = moo.states(
  {
    main: {
      WHITESPACE: {
        match: /\s+/,
        lineBreaks: true,
      },
      COMMENT: /\/\/.*/,
      IDENTIFIER: {
        match: /[a-zA-Z_][a-zA-Z0-9_]*/,
        type: moo.keywords(Keywords),
      },
      NUMBER_LITERAL: {
        // @ts-ignore
        match: /\d+/,
        value: (text) => parseInt(text, 10),
      },
      STRING_LITERAL: {
        match: /(?:'(?:[^']|\\')*')|(?:"(?:[^"]|\\")*")/,
        value: (text) => text.substring(1, text.length - 1),
      },
      COLON: ':',
      SEMICOLON: ';',
      COMMA: ',',
      DOT: '.',
      LPAREN: '(',
      RPAREN: ')',
      LBRACE: '{',
      RBRACE: '}',
      LBRACKET: '[',
      RBRACKET: ']',
      AND: '&&',
      OR: '||',
      ADD: '+',
      SUB: '-', // Note: must be after NUMERIC_LITERAL
      EXP: '**', // Note: must be before MUL
      MUL: '*',
      DIV: '/',
      // Note: order matters in the comparison operators!
      EQ: '==',
      NE: '!=', // Note: must be before NOT
      GTE: '>=',
      LTE: '<=',
      GT: '>',
      LT: '<',
      ASSIGN: '=',
      NOT: '!',
    },
  },
  'main'
);

const TOKEN_TYPES_TO_DISCARD = ['WHITESPACE', 'COMMENT'];

class Lexer {
  constructor(private readonly mooLexer: moo.Lexer) {}

  next(): Token | undefined {
    let token: Token | undefined;
    do {
      token = this.mooLexer.next();
    } while (
      token &&
      token.type &&
      TOKEN_TYPES_TO_DISCARD.includes(token.type)
    );
    // console.log(`>> TOKEN ${token?.type}`);
    return token;
  }

  save(): moo.LexerState {
    return this.mooLexer.save();
  }

  reset(chunk?: string, state?: moo.LexerState) {
    this.mooLexer.reset(chunk, state);
  }

  has(tokenType: string) {
    return this.mooLexer.has(tokenType);
  }

  formatError(token: Token, message?: string) {
    return this.mooLexer.formatError(token, message);
  }
}

const lexer = new Lexer(mooLexer);
export default lexer;
