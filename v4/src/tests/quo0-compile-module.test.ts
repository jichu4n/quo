import fs from 'fs-extra';
import path from 'path';
import {compileQuoFile} from '../quo0';

function getTestInputFiles() {
  const testDir = path.join(__dirname, '..', '..', 'src', 'tests', 'testdata');
  return fs
    .readdirSync(testDir)
    .filter((file) => file.endsWith('.quo'))
    .map((file) => path.join(testDir, file));
}

describe('quo0-compile-module', () => {
  for (const inputFile of getTestInputFiles()) {
    test(inputFile, async () => {
      await compileQuoFile(inputFile);
    });
  }
});
