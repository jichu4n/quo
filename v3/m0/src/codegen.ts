import {lines} from 'lines-builder';
import {ClassDef, FnDef, ModuleDef} from './ast';

export function generateHeaderFile(module: ModuleDef): string {
  const l = lines();

  l.append(`#include "quo-rt.h"`);
  l.append(
    ...module.importDecls.map(({moduleName}) => `#include "${moduleName}.h"`),
    null
  );

  l.append(...module.classDefs.map(generateClassDef), null);

  if (module.fnDefs.length) {
    l.append(
      'extern "C" {',
      null,
      ...module.fnDefs.map(generateFnDecl),
      null,
      '}'
    );
  }

  return l.toString().trim();
}

function generateClassDef(classDef: ClassDef) {
  return lines(
    `struct ${classDef.name} {`,
    lines(
      {indent: 2},
      ...classDef.props.map(({typeString, name}) => `${typeString}* ${name};`)
    ),
    `};`
  );
}

function generateFnDecl(fnDef: FnDef) {
  return [
    fnDef.returnTypeString ? `${fnDef.returnTypeString}*` : 'void',
    ' ',
    fnDef.name,
    '(',
    fnDef.params
      .map(({typeString, name}) => `${typeString}* ${name}`)
      .join(', '),
    ');',
  ].join('');
}

if (require.main === module) {
  (async () => {
    const {parse} = await import('./parser');
    const {program} = await import('commander');
    const fs = await import('fs-extra');

    program.argument('<input-file>', 'Quo source file');
    program.parse();

    const sourceCode = await fs.readFile(program.args[0], 'utf-8');
    const moduleDef = parse(sourceCode);

    console.log(generateHeaderFile(moduleDef));
  })();
}
