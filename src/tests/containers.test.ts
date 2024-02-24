import {setRawStr, loadQuoWasmModule} from '../quo-driver';
import {expectUsedMemChunks} from './test-helpers';

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
      _memoryInit,
      _strNew,
      _strDelete,
      _strFromRaw,
      _arrNew,
      _arrDelete,
      _arrPush,
      _arrGet,
      _arrSet,
      _dictNew,
      _dictDelete,
      _dictFind,
      _dictGet,
      _dictSet,
      _malloc,
    } = fns;

    _memoryInit(heapStart, heapEnd);
    return {
      _strNew,
      _strDelete,
      _strFromRaw,
      _arrNew,
      _arrDelete,
      _arrPush,
      _arrGet,
      _arrSet,
      _dictNew,
      _dictDelete,
      _dictFind,
      _dictGet,
      _dictSet,
      _malloc,
      wasmMemory,
    };
  };

  describe(`stage ${stage} strings`, () => {
    test('array push', async function () {
      const {_arrNew, _arrDelete, _arrPush, wasmMemory} = await setup();
      expectUsedMemChunks(wasmMemory, heapStart, 0);
      const arr = _arrNew(0);
      expectUsedMemChunks(wasmMemory, heapStart, 2); // header + data
      expect(toArr(wasmMemory, arr)).toStrictEqual({
        size: 4,
        len: 0,
        data: [],
      });

      for (let i = 0; i < 100; i++) {
        _arrPush(arr, i);
        expect(toArr(wasmMemory, arr)).toMatchObject({
          len: i + 1,
          data: Array(i + 1)
            .fill(0)
            .map((_, j) => j),
        });
      }
      expectUsedMemChunks(wasmMemory, heapStart, 2);
      _arrDelete(arr);
      expectUsedMemChunks(wasmMemory, heapStart, 0);
    });
    test('array get and set', async function () {
      const {_arrNew, _arrDelete, _arrGet, _arrSet, _arrPush, wasmMemory} =
        await setup();
      expectUsedMemChunks(wasmMemory, heapStart, 0);
      const arr = _arrNew(0);
      for (let i = 0; i < 100; i++) {
        _arrPush(arr, i);
      }

      for (let i = 0; i < 100; i++) {
        expect(_arrGet(arr, i)).toStrictEqual(i);
      }
      expect(() => _arrGet(arr, 100)).toThrow();

      for (let i = 0; i < 100; i++) {
        _arrSet(arr, i, _arrGet(arr, i) * 2);
      }
      expect(() => _arrSet(arr, 100, 200)).toThrow();
      for (let i = 0; i < 100; i++) {
        expect(_arrGet(arr, i)).toStrictEqual(i * 2);
      }

      _arrDelete(arr);
      expectUsedMemChunks(wasmMemory, heapStart, 0);
    });
    test('array free values', async function () {
      const {_arrNew, _arrDelete, _arrPush, _malloc, wasmMemory} =
        await setup();
      expectUsedMemChunks(wasmMemory, heapStart, 0);
      const arr = _arrNew(0);
      for (let i = 0; i < 10; i++) {
        _arrPush(arr, _malloc(1));
      }
      expectUsedMemChunks(wasmMemory, heapStart, 2 + 10);
      _arrDelete(arr);
      expectUsedMemChunks(wasmMemory, heapStart, 10);
    });
    test('dict set and get', async function () {
      const {
        _strFromRaw,
        _strDelete,
        _dictNew,
        _dictDelete,
        _dictFind,
        _dictGet,
        _dictSet,
        wasmMemory,
      } = await setup();
      expectUsedMemChunks(wasmMemory, heapStart, 0);
      const dict = _dictNew(0);
      expectUsedMemChunks(wasmMemory, heapStart, 5); // dict header, 2x array headers, 2x array data

      setRawStr(wasmMemory, 4096, 'foo');
      const str1 = _strFromRaw(4096);
      const str2 = _strFromRaw(4096);
      setRawStr(wasmMemory, 4096 + 10, 'bar');
      const str3 = _strFromRaw(4096 + 10);

      expect(_dictFind(dict, str1)).toStrictEqual(-1);
      expect(_dictGet(dict, str1)).toStrictEqual(0);
      expect(_dictFind(dict, str3)).toStrictEqual(-1);
      expect(_dictGet(dict, str3)).toStrictEqual(0);

      _dictSet(dict, str1, 42, 0);
      expect(_dictFind(dict, str1)).toStrictEqual(0);
      expect(_dictGet(dict, str1)).toStrictEqual(42);
      expect(_dictFind(dict, str2)).toStrictEqual(0);
      expect(_dictGet(dict, str2)).toStrictEqual(42);
      expect(_dictFind(dict, str3)).toStrictEqual(-1);
      expect(_dictGet(dict, str3)).toStrictEqual(0);

      _dictSet(dict, str3, 33, 0);
      expect(_dictFind(dict, str1)).toStrictEqual(0);
      expect(_dictGet(dict, str1)).toStrictEqual(42);
      expect(_dictFind(dict, str2)).toStrictEqual(0);
      expect(_dictGet(dict, str2)).toStrictEqual(42);
      expect(_dictFind(dict, str3)).toStrictEqual(1);
      expect(_dictGet(dict, str3)).toStrictEqual(33);

      _strDelete(str1);
      _strDelete(str2);
      _strDelete(str3);
      _dictDelete(dict, 0);
      expectUsedMemChunks(wasmMemory, heapStart, 0);
    });
  });
}
