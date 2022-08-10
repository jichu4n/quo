import {Grammar, Parser, ParserOptions} from 'nearley';
import {ModuleDef} from './ast';
import grammar from './grammar';

export function createParser(opts?: ParserOptions) {
  return new Parser(Grammar.fromCompiled(grammar), opts);
}

/** Entry point for parsing an input string into an AST module. */
export function parse(
  input: string,
  opts?: {sourceFileName?: string} & ParserOptions
): ModuleDef {
  const parser = createParser(opts);
  try {
    parser.feed(input);
  } catch (e: any) {
    if ('token' in e) {
      throw new Error(
        // Hack to reduce verbosity of nearley error messages...
        e.message.replace(
          / Instead, I was expecting to see one of the following:$[\s\S]*/gm,
          ''
        )
      );
    } else {
      throw e;
    }
  }
  if (parser.results.length === 0) {
    throw new Error(`Unexpected end of input`);
  } else if (parser.results.length !== 1) {
    throw new Error(
      `${parser.results.length} parse trees:\n\n` +
        parser.results
          .map(
            (tree, idx) =>
              `Parse tree #${idx + 1}:\n${JSON.stringify(tree, null, 4)}`
          )
          .join('\n\n')
    );
  }
  const moduleDef = parser.results[0] as ModuleDef;
  if (opts?.sourceFileName) {
    moduleDef.name = opts.sourceFileName;
  }
  return moduleDef;
}

if (require.main === module) {
  (async () => {
    const {program} = await import('commander');
    const fs = await import('fs-extra');

    program.argument('<input-file>', 'Quo source file to parse');
    program.parse();

    const sourceFileName = program.args[0];
    const sourceCode = await fs.readFile(sourceFileName, 'utf-8');
    const moduleDef = parse(sourceCode, {sourceFileName});

    console.log(JSON.stringify(moduleDef, null, 2));
  })();
}
