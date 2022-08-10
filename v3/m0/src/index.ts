import {codegen, getFileNameWithoutExt} from './codegen';
import path from 'path';
import execa from 'execa';
import glob from 'glob-promise';

function getObjectFilePath(cppFilePath: string) {
  return path.join(
    path.dirname(cppFilePath),
    `${path.basename(cppFilePath, '.cpp')}.o`
  );
}

const RT_DIR_PATH = path.resolve(path.join(__dirname, '..', '..', 'rt'));

export async function build(
  sourceFilePaths: Array<string>,
  binaryFilePath: string,
  {
    cxx,
    cxxFlags,
    linkFlags,
  }: {cxx?: string; cxxFlags?: Array<string>; linkFlags?: Array<string>} = {}
) {
  if (!cxx) {
    cxx = process.env['CXX'] || 'g++';
  }
  const objectFilePaths: Array<string> = [];
  for (const sourceFilePath of sourceFilePaths) {
    console.log(`> ${sourceFilePath}`);
    const {cppFilePath} = await codegen(sourceFilePath);
    const objectFilePath = getObjectFilePath(cppFilePath);
    const cxxArgs = [
      '-c',
      '-o',
      objectFilePath,
      `-I${RT_DIR_PATH}`,
      ...(cxxFlags ?? []),
      cppFilePath,
    ];
    console.log(`    - ${cxx} ${cxxArgs.join(' ')}`);
    await execa(cxx, cxxArgs, {stderr: 'inherit', stdout: 'inherit'});
    objectFilePaths.push(objectFilePath);
  }

  const linkArgs = [
    '-o',
    binaryFilePath,
    ...(linkFlags ?? []),
    ...objectFilePaths,
    ...(await glob(path.join(RT_DIR_PATH, '*.cpp'))),
  ];
  console.log(`> ${cxx} ${linkArgs.join(' ')}`);
  await execa(cxx, linkArgs, {stderr: 'inherit', stdout: 'inherit'});
}

if (require.main === module) {
  (async () => {
    const {program} = await import('commander');

    program.option('-o <binary>', 'Output binary file');
    program.argument('<input-files...>', 'Quo source files');
    program.parse();

    const sourceFilePaths = program.args;
    const binaryFilePath = program.opts()['o'];
    if (!binaryFilePath) {
      throw new Error(`Please specify output binary file with -o`);
    }
    await build(sourceFilePaths, binaryFilePath);
  })();
}
