import {setRawStr, loadQuoWasmModule} from '../quo-driver';
import {expectUsedChunks} from './memory.test';

const stages = ['1a', '1b', '1c'];

const heapStart = 32;
const heapEnd = 2048;

interface Arr {
  size: number;
  len: number;
  data: Array<number>;
}

function toArr(memory: WebAssembly.Memory, address: number): Arr {
  const buffer = Buffer.from(memory.buffer);
  const size = buffer.readUInt32LE(address + 0);
  const len = buffer.readUInt32LE(address + 4);
  const dataAddress = buffer.readUInt32LE(address + 8);
  const data: Array<number> = [];
  for (let i = 0; i < len; i++) {
    data.push(buffer.readUInt32LE(dataAddress + i * 4));
  }
  return {size, len, data};
}

for (const stage of stages) {
  const setup = async () => {
    const {wasmMemory, fns} = await loadQuoWasmModule(stage);
    const {
      memoryInit,
      strNew,
      strDelete,
      strFromRaw,
      arrNew,
      arrDelete,
      arrPush,
      arrGet,
      arrSet,
      dictNew,
      dictDelete,
      dictFind,
      dictGet,
      dictSet,
      malloc,
    } = fns;

    memoryInit(heapStart, heapEnd);
    return {
      strNew,
      strDelete,
      strFromRaw,
      arrNew,
      arrDelete,
      arrPush,
      arrGet,
      arrSet,
      dictNew,
      dictDelete,
      dictFind,
      dictGet,
      dictSet,
      malloc,
      wasmMemory,
    };
  };

  describe(`stage ${stage} strings`, () => {
    test('array push', async function () {
      const {arrNew, arrDelete, arrPush, wasmMemory} = await setup();
      expectUsedChunks(wasmMemory, heapStart, 0);
      const arr = arrNew(0);
      expectUsedChunks(wasmMemory, heapStart, 2); // header + data
      expect(toArr(wasmMemory, arr)).toStrictEqual({
        size: 4,
        len: 0,
        data: [],
      });

      for (let i = 0; i < 100; i++) {
        arrPush(arr, i);
        expect(toArr(wasmMemory, arr)).toMatchObject({
          len: i + 1,
          data: Array(i + 1)
            .fill(0)
            .map((_, j) => j),
        });
      }
      expectUsedChunks(wasmMemory, heapStart, 2);
      arrDelete(arr, 0);
      expectUsedChunks(wasmMemory, heapStart, 0);
    });
    test('array get and set', async function () {
      const {arrNew, arrDelete, arrGet, arrSet, arrPush, wasmMemory} =
        await setup();
      expectUsedChunks(wasmMemory, heapStart, 0);
      const arr = arrNew(0);
      for (let i = 0; i < 100; i++) {
        arrPush(arr, i);
      }

      for (let i = 0; i < 100; i++) {
        expect(arrGet(arr, i)).toStrictEqual(i);
      }
      expect(() => arrGet(arr, 100)).toThrow();

      for (let i = 0; i < 100; i++) {
        arrSet(arr, i, arrGet(arr, i) * 2);
      }
      expect(() => arrSet(arr, 100, 200)).toThrow();
      for (let i = 0; i < 100; i++) {
        expect(arrGet(arr, i)).toStrictEqual(i * 2);
      }

      arrDelete(arr, 0);
      expectUsedChunks(wasmMemory, heapStart, 0);
    });
    test('array free values', async function () {
      const {arrNew, arrDelete, arrPush, malloc, wasmMemory} = await setup();
      expectUsedChunks(wasmMemory, heapStart, 0);
      const arr = arrNew(0);
      for (let i = 0; i < 10; i++) {
        arrPush(arr, malloc(1));
      }
      expectUsedChunks(wasmMemory, heapStart, 2 + 10);
      arrDelete(arr, 1);
      expectUsedChunks(wasmMemory, heapStart, 0);
    });
    test('dict set and get', async function () {
      const {
        strFromRaw,
        strDelete,
        dictNew,
        dictDelete,
        dictFind,
        dictGet,
        dictSet,
        wasmMemory,
      } = await setup();
      expectUsedChunks(wasmMemory, heapStart, 0);
      const dict = dictNew(0);
      expectUsedChunks(wasmMemory, heapStart, 5); // dict header, 2x array headers, 2x array data

      setRawStr(wasmMemory, 4096, 'foo');
      const str1 = strFromRaw(4096);
      const str2 = strFromRaw(4096);
      setRawStr(wasmMemory, 4096 + 10, 'bar');
      const str3 = strFromRaw(4096 + 10);

      expect(dictFind(dict, str1)).toStrictEqual(-1);
      expect(dictGet(dict, str1)).toStrictEqual(0);
      expect(dictFind(dict, str3)).toStrictEqual(-1);
      expect(dictGet(dict, str3)).toStrictEqual(0);

      dictSet(dict, str1, 42, 0);
      expect(dictFind(dict, str1)).toStrictEqual(0);
      expect(dictGet(dict, str1)).toStrictEqual(42);
      expect(dictFind(dict, str2)).toStrictEqual(0);
      expect(dictGet(dict, str2)).toStrictEqual(42);
      expect(dictFind(dict, str3)).toStrictEqual(-1);
      expect(dictGet(dict, str3)).toStrictEqual(0);

      dictSet(dict, str3, 33, 0);
      expect(dictFind(dict, str1)).toStrictEqual(0);
      expect(dictGet(dict, str1)).toStrictEqual(42);
      expect(dictFind(dict, str2)).toStrictEqual(0);
      expect(dictGet(dict, str2)).toStrictEqual(42);
      expect(dictFind(dict, str3)).toStrictEqual(1);
      expect(dictGet(dict, str3)).toStrictEqual(33);

      strDelete(str1);
      strDelete(str2);
      strDelete(str3);
      dictDelete(dict, 0);
      expectUsedChunks(wasmMemory, heapStart, 0);
    });
  });
}
