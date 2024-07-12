import {loadQuoWasmModule, setRawStr} from '../quo-driver';
import {
  compileAndLoadAsModule,
  expectUsedMemChunks,
  getStr,
} from './test-helpers';

const stages = ['2a'];

describe.each(stages)('stage %s compile class', (stage) => {
  let fns: Record<string, CallableFunction>;
  let deleteTempFiles: () => Promise<void>;

  afterEach(async () => {
    await deleteTempFiles();
  });

  test('empty class', async () => {
    ({fns, deleteTempFiles} = await compileAndLoadAsModule(
      stage,
      `class A { }`,
      'empty-class'
    ));
  });
});
