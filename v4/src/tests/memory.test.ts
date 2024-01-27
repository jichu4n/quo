import {setupWasmModule} from '../quo-driver';

const stages = ['1a'];

const memoryStart = 32;
const memoryEnd = 1024;
const memorySize = memoryEnd - memoryStart;
const headerSize = 16;

export interface Chunk {
  address: number;
  size: number;
  next: number;
  prev: number;
  used: boolean;
}

function toChunk(buffer: Buffer, address: number): Chunk {
  return {
    address,
    size: buffer.readUInt32LE(address + 0),
    next: buffer.readUInt32LE(address + 4),
    prev: buffer.readUInt32LE(address + 8),
    used: Boolean(buffer.readUInt8(address + 12)),
  };
}

export function getChunks(memory: WebAssembly.Memory): Array<Chunk> {
  const buffer = Buffer.from(memory.buffer);
  const head = toChunk(buffer, memoryStart);
  const chunks = [head];
  let chunk = head;
  while (chunk.next !== 0) {
    chunk = toChunk(buffer, chunk.next);
    chunks.push(chunk);
  }
  return chunks;
}

export function expectUsedChunks(memory: WebAssembly.Memory, num: number) {
  expect(getChunks(memory).filter((chunk) => chunk.used)).toHaveLength(num);
}


for (const stage of stages) {
  const setup = async () => {
    const {wasmMemory, fns} = await setupWasmModule(stage);
    const {memoryInit, malloc, free} = fns;

    memoryInit(memoryStart, memoryEnd);
    return {malloc, free, wasmMemory};
  };

  describe(`stage ${stage} malloc & free`, () => {
    test('initial state', async () => {
      let {wasmMemory} = await setup();
      expect(getChunks(wasmMemory)).toStrictEqual([
        {
          address: memoryStart,
          size: memorySize - headerSize,
          next: 0,
          prev: 0,
          used: false,
        },
      ]);
    });
    test('allocate and split', async () => {
      const {wasmMemory, malloc} = await setup();
      const p1 = malloc(1);
      expect(p1).toStrictEqual(memoryStart + headerSize);
      expect(getChunks(wasmMemory)).toStrictEqual([
        {
          address: p1 - headerSize,
          size: 16,
          next: p1 + 16,
          prev: 0,
          used: true,
        },
        {
          address: p1 + 16,
          size: memoryEnd - (p1 + 16) - headerSize,
          next: 0,
          prev: p1 - headerSize,
          used: false,
        },
      ]);
      const p2 = malloc(50);
      expect(p2).toStrictEqual(p1 + 16 + headerSize);
      expect(getChunks(wasmMemory)).toStrictEqual([
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
          size: memoryEnd - (p2 + 64) - 16,
          next: 0,
          prev: p2 - headerSize,
          used: false,
        },
      ]);
      // Now use up the entire remaining space
      const p3 = malloc(memoryEnd - (p2 + 64) - 16);
      expect(p3).toStrictEqual(p2 + 64 + headerSize);
      expect(getChunks(wasmMemory)).toStrictEqual([
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
          size: memoryEnd - p3,
          next: 0,
          prev: p2 - headerSize,
          used: true,
        },
      ]);
      // Now there should be no more space
      expect(() => malloc(1)).toThrow();
    });
    test('allocate invalid sizes', async () => {
      const {wasmMemory, malloc} = await setup();
      let p = malloc(0);
      expect(p).toStrictEqual(0);
      expect(() => malloc(1024)).toThrow();
    });
    test('free with and without merging', async () => {
      const {wasmMemory, malloc, free} = await setup();
      const p1 = malloc(1),
        p2 = malloc(1),
        p3 = malloc(1);
      expect(getChunks(wasmMemory)).toStrictEqual([
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
          size: memoryEnd - (p3 + 16) - 16,
          next: 0,
          prev: p3 - headerSize,
          used: false,
        },
      ]);
      // Free p2 -- should not merge.
      free(p2);
      expect(getChunks(wasmMemory)).toStrictEqual([
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
          size: memoryEnd - (p3 + 16) - 16,
          next: 0,
          prev: p3 - headerSize,
          used: false,
        },
      ]);
      // Free p1 -- should merge with p2.
      free(p1);
      expect(getChunks(wasmMemory)).toStrictEqual([
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
          size: memoryEnd - (p3 + 16) - 16,
          next: 0,
          prev: p3 - headerSize,
          used: false,
        },
      ]);
      // Free p3 -- should all merge into one big chunk.
      free(p3);
      expect(getChunks(wasmMemory)).toStrictEqual([
        {
          address: memoryStart,
          size: memorySize - headerSize,
          next: 0,
          prev: 0,
          used: false,
        },
      ]);
    });
  });
}
