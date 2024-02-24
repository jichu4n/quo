import {loadQuoWasmModule} from '../quo-driver';
import {getMemChunks} from './test-helpers';

const stages = ['1a', '1b', '1c'];

const heapStart = 32;
const heapEnd = 1024;
const heapSize = heapEnd - heapStart;
const headerSize = 16;

for (const stage of stages) {
  const setup = async () => {
    const {wasmMemory, fns} = await loadQuoWasmModule(stage);
    const {_memoryInit, _malloc, _free} = fns;

    _memoryInit(heapStart, heapEnd);
    return {_malloc, _free, wasmMemory};
  };

  describe(`stage ${stage} _malloc & _free`, () => {
    test('initial state', async () => {
      let {wasmMemory} = await setup();
      expect(getMemChunks(wasmMemory, heapStart)).toStrictEqual([
        {
          address: heapStart,
          size: heapSize - headerSize,
          next: 0,
          prev: 0,
          used: false,
        },
      ]);
    });
    test('allocate and split', async () => {
      const {wasmMemory, _malloc} = await setup();
      const p1 = _malloc(1);
      expect(p1).toStrictEqual(heapStart + headerSize);
      expect(getMemChunks(wasmMemory, heapStart)).toStrictEqual([
        {
          address: p1 - headerSize,
          size: 16,
          next: p1 + 16,
          prev: 0,
          used: true,
        },
        {
          address: p1 + 16,
          size: heapEnd - (p1 + 16) - headerSize,
          next: 0,
          prev: p1 - headerSize,
          used: false,
        },
      ]);
      const p2 = _malloc(50);
      expect(p2).toStrictEqual(p1 + 16 + headerSize);
      expect(getMemChunks(wasmMemory, heapStart)).toStrictEqual([
        {
          address: p1 - headerSize,
          size: 16,
          next: p1 + 16,
          prev: 0,
          used: true,
        },
        {
          address: p2 - headerSize,
          size: 64,
          next: p2 + 64,
          prev: p1 - headerSize,
          used: true,
        },
        {
          address: p2 + 64,
          size: heapEnd - (p2 + 64) - 16,
          next: 0,
          prev: p2 - headerSize,
          used: false,
        },
      ]);
      // Now use up the entire remaining space
      const p3 = _malloc(heapEnd - (p2 + 64) - 16);
      expect(p3).toStrictEqual(p2 + 64 + headerSize);
      expect(getMemChunks(wasmMemory, heapStart)).toStrictEqual([
        {
          address: p1 - headerSize,
          size: 16,
          next: p1 + 16,
          prev: 0,
          used: true,
        },
        {
          address: p2 - headerSize,
          size: 64,
          next: p2 + 64,
          prev: p1 - headerSize,
          used: true,
        },
        {
          address: p3 - headerSize,
          size: heapEnd - p3,
          next: 0,
          prev: p2 - headerSize,
          used: true,
        },
      ]);
      // Now there should be no more space
      expect(() => _malloc(1)).toThrow();
    });
    test('allocate invalid sizes', async () => {
      const {wasmMemory, _malloc} = await setup();
      let p = _malloc(0);
      expect(p).toStrictEqual(0);
      expect(() => _malloc(1024)).toThrow();
    });
    test('free with and without merging', async () => {
      const {wasmMemory, _malloc, _free} = await setup();
      const p1 = _malloc(1),
        p2 = _malloc(1),
        p3 = _malloc(1);
      expect(getMemChunks(wasmMemory, heapStart)).toStrictEqual([
        {
          address: p1 - headerSize,
          size: 16,
          next: p1 + 16,
          prev: 0,
          used: true,
        },
        {
          address: p2 - headerSize,
          size: 16,
          next: p2 + 16,
          prev: p1 - headerSize,
          used: true,
        },
        {
          address: p3 - headerSize,
          size: 16,
          next: p3 + 16,
          prev: p2 - headerSize,
          used: true,
        },
        {
          address: p3 + 16,
          size: heapEnd - (p3 + 16) - 16,
          next: 0,
          prev: p3 - headerSize,
          used: false,
        },
      ]);
      // Free p2 -- should not merge.
      _free(p2);
      expect(getMemChunks(wasmMemory, heapStart)).toStrictEqual([
        {
          address: p1 - headerSize,
          size: 16,
          next: p1 + 16,
          prev: 0,
          used: true,
        },
        {
          address: p2 - headerSize,
          size: 16,
          next: p2 + 16,
          prev: p1 - headerSize,
          used: false,
        },
        {
          address: p3 - headerSize,
          size: 16,
          next: p3 + 16,
          prev: p2 - headerSize,
          used: true,
        },
        {
          address: p3 + 16,
          size: heapEnd - (p3 + 16) - 16,
          next: 0,
          prev: p3 - headerSize,
          used: false,
        },
      ]);
      // Free p1 -- should merge with p2.
      _free(p1);
      expect(getMemChunks(wasmMemory, heapStart)).toStrictEqual([
        {
          address: p1 - headerSize,
          size: p3 - headerSize - p1,
          next: p3 - headerSize,
          prev: 0,
          used: false,
        },
        {
          address: p3 - headerSize,
          size: 16,
          next: p3 + 16,
          prev: p1 - headerSize,
          used: true,
        },
        {
          address: p3 + 16,
          size: heapEnd - (p3 + 16) - 16,
          next: 0,
          prev: p3 - headerSize,
          used: false,
        },
      ]);
      // Free p3 -- should all merge into one big chunk.
      _free(p3);
      expect(getMemChunks(wasmMemory, heapStart)).toStrictEqual([
        {
          address: heapStart,
          size: heapSize - headerSize,
          next: 0,
          prev: 0,
          used: false,
        },
      ]);
    });
  });
}
