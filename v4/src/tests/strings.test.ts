import {loadQuoWasmModule, setRawStr} from '../quo-driver';
import {expectUsedMemChunks, getStr, getStrObject} from './test-helpers';

const stages = ['1a', '1b', '1c'];

const heapStart = 32;
const heapEnd = 2048;

for (const stage of stages) {
  const setup = async () => {
    const {wasmMemory, fns} = await loadQuoWasmModule(stage);
    const {
      memoryInit,
      strNew,
      strDelete,
      strFromRaw,
      strFlatten,
      strToRaw,
      strMerge,
      strPushRaw,
      strPushChar,
      strCmpRaw,
      strCmp,
    } = fns;

    memoryInit(heapStart, heapEnd);
    return {
      strNew,
      strDelete,
      strFromRaw,
      strFlatten,
      strToRaw,
      strMerge,
      strPushRaw,
      strPushChar,
      strCmp,
      strCmpRaw,
      wasmMemory,
    };
  };

  describe(`stage ${stage} strings`, () => {
    test('create and free empty string', async function () {
      const {strNew, strDelete, wasmMemory} = await setup();
      expectUsedMemChunks(wasmMemory, heapStart, 0);
      const str1 = strNew(0);
      expect(getStrObject(wasmMemory, str1)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      expectUsedMemChunks(wasmMemory, heapStart, 2);
      strDelete(str1);
      expectUsedMemChunks(wasmMemory, heapStart, 0);
      const str2 = strNew(0);
      expect(getStrObject(wasmMemory, str2)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      expectUsedMemChunks(wasmMemory, heapStart, 2);
      expect(str1).toStrictEqual(str2);
    });
    test('create and free string from raw string', async function () {
      const {strFromRaw, strDelete, wasmMemory} = await setup();
      expectUsedMemChunks(wasmMemory, heapStart, 0);
      setRawStr(wasmMemory, 4096, 'foo');
      const str1 = strFromRaw(4096);
      expectUsedMemChunks(wasmMemory, heapStart, 2);
      expect(getStrObject(wasmMemory, str1)).toStrictEqual({
        len: 3,
        chunks: [{size: 32, len: 3, data: 'foo'}],
      });
      strDelete(str1);
      expectUsedMemChunks(wasmMemory, heapStart, 0);

      const str2 = strFromRaw(4096 + 10); // nothing there
      expectUsedMemChunks(wasmMemory, heapStart, 2);
      expect(getStrObject(wasmMemory, str2)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      strDelete(str2);
      expectUsedMemChunks(wasmMemory, heapStart, 0);
    });
    test('merge strings', async function () {
      const {strNew, strFromRaw, strMerge, strDelete, strFlatten, wasmMemory} =
        await setup();

      const str0 = strNew(0);
      setRawStr(wasmMemory, 4096, 'foo');
      const str1 = strFromRaw(4096);
      setRawStr(wasmMemory, 4096 + 10, 'bar');
      const str2 = strFromRaw(4096 + 10);
      // Empty + non-empty
      strMerge(str0, str1);
      expect(getStrObject(wasmMemory, str0)).toStrictEqual({
        len: 3,
        chunks: [{size: 32, len: 3, data: 'foo'}],
      });
      // Non-empty + non-empty, but enough space in first string
      strMerge(str0, str2);
      expect(getStrObject(wasmMemory, str0)).toStrictEqual({
        len: 6,
        chunks: [{size: 32, len: 6, data: 'foobar'}],
      });
      expectUsedMemChunks(wasmMemory, heapStart, 2);

      const str3 = strNew(0);
      const str4 = strNew(0);
      // Empty + empty
      strMerge(str3, str4);
      expect(getStrObject(wasmMemory, str3)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      // Non-empty + empty
      strMerge(str0, str3);
      expect(getStrObject(wasmMemory, str0)).toStrictEqual({
        len: 6,
        chunks: [{size: 32, len: 6, data: 'foobar'}],
      });
      expectUsedMemChunks(wasmMemory, heapStart, 2);

      // Non-empty + large non-empt
      setRawStr(wasmMemory, 4096 + 20, 'a'.repeat(100));
      const str5 = strFromRaw(4096 + 20);
      expectUsedMemChunks(wasmMemory, heapStart, 4);
      strMerge(str0, str5);
      expect(getStrObject(wasmMemory, str0)).toStrictEqual({
        len: 106,
        chunks: [
          {size: 32, len: 6, data: 'foobar'},
          {size: 100, len: 100, data: 'a'.repeat(100)},
        ],
      });
      strFlatten(str0);
      expect(getStrObject(wasmMemory, str0)).toStrictEqual({
        len: 106,
        chunks: [{size: 108, len: 106, data: 'foobar' + 'a'.repeat(100)}],
      });
      expectUsedMemChunks(wasmMemory, heapStart, 2);

      // Lots of non-empty. Should never exceed 8 chunks.
      const str6 = strFromRaw(4096);
      for (let i = 0; i < 150; ++i) {
        const str7 = strFromRaw(4096);
        strMerge(str6, str7);
        expect(
          getStrObject(wasmMemory, str6).chunks.length
        ).toBeLessThanOrEqual(8);
      }
    });
    test('push strings', async function () {
      const {strNew, strPushRaw, wasmMemory} = await setup();
      setRawStr(wasmMemory, 4096, 'A');
      const str1 = strNew(0);
      for (let i = 1; i <= 500; ++i) {
        strPushRaw(str1, 4096);
        expect(getStrObject(wasmMemory, str1)).toMatchObject({
          len: i,
        });
        expect(getStr(wasmMemory, str1)).toStrictEqual('A'.repeat(i));
      }
    });
    test('compare strings', async function () {
      const {strNew, strPushRaw, strFromRaw, strCmp, strCmpRaw, wasmMemory} =
        await setup();
      // Basic strings.
      setRawStr(wasmMemory, 4096, 'foo');
      const str1 = strFromRaw(4096);
      const str2 = strFromRaw(4096);
      expect(strCmp(str1, str1)).toStrictEqual(0);
      expect(strCmpRaw(str1, 4096)).toStrictEqual(0);
      expect(strCmp(str1, str2)).toStrictEqual(0);

      setRawStr(wasmMemory, 4096, 'foobar');
      const str3 = strFromRaw(4096);
      expect(strCmp(str1, str3)).toBeLessThan(0);
      expect(strCmp(str3, str1)).toBeGreaterThan(0);

      const str4 = strFromRaw(4096 + 3); // "bar"
      expect(strCmp(str1, str4)).toBeGreaterThan(0);
      expect(strCmp(str4, str1)).toBeLessThan(0);

      // Empty strings.
      expect(strCmp(strNew(0), strNew(0))).toStrictEqual(0);

      // 1 chunk vs 2 chunks.
      setRawStr(wasmMemory, 4096 + 10, 'foobar' + 'a'.repeat(100));
      strPushRaw(str3, 4096 + 10 + 6);
      expect(getStrObject(wasmMemory, str3).chunks.length).toStrictEqual(2); // 'foobar' + 'a'.repeat(100)
      expect(strCmp(str3, strFromRaw(4096 + 10))).toStrictEqual(0);
    });
  });
}
