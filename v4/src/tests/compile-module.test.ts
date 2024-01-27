import fs from 'fs-extra';
import path from 'path';
import {compileFiles} from '../quo-driver';

const stages = ['0', '1a'];

function getTestInputFiles() {
  const testDir = path.join(__dirname, '..', '..', 'src', 'tests', 'testdata');
  return fs
    .readdirSync(testDir)
    .filter((file) => file.endsWith('.quo'))
    .map((file) => path.join(testDir, file));
}

for (const stage of stages) {
  describe(`stage ${stage} compile module`, () => {
    for (const inputFile of getTestInputFiles()) {
      test(inputFile, async () => {
        const outputFile = path.format({
          ...path.parse(inputFile),
          base: '',
          ext: `.stage${stage}.wasm`,
        });
        await compileFiles(stage, [inputFile], outputFile);
      });
    }
  });
}
