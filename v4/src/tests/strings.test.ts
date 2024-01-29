import {setWasmString, loadQuoWasmModule} from '../quo-driver';
import {expectUsedChunks} from './memory.test';

const stages = ['1a'];

const heapStart = 32;
const heapEnd = 2048;
const strChunkHeaderSize = 12;

interface RawStrHeader {
  address: number;
  len: number;
  next: number;
}

interface StrChunk {
  size: number;
  len: number;
  data: string;
}

interface RawStrChunk extends StrChunk {
  address: number;
  next: number;
}

interface Str {
  len: number;
  chunks: Array<StrChunk>;
}

function toRawStrHeader(buffer: Buffer, address: number): RawStrHeader {
  return {
    address,
    len: buffer.readUInt32LE(address + 0),
    next: buffer.readUInt32LE(address + 4),
  };
}

function toRawStrChunk(buffer: Buffer, address: number): RawStrChunk {
  const chunkLen = buffer.readUInt32LE(address + 4);
  const dataStart = address + strChunkHeaderSize;
  const dataEnd = dataStart + chunkLen;
  const data = buffer.toString('utf8', dataStart, dataEnd);
  if (data.length !== chunkLen) {
    throw new Error(
      `String chunk length mismatch at address ${address}: ` +
        `expected ${chunkLen}, actual length ${data.length}`
    );
  }
  return {
    address,
    size: buffer.readUInt32LE(address + 0),
    len: chunkLen,
    next: buffer.readUInt32LE(address + 8),
    data,
  };
}

function getStr(memory: WebAssembly.Memory, address: number): Str {
  const buffer = Buffer.from(memory.buffer);
  const header = toRawStrHeader(buffer, address);
  const chunks = [];
  let chunkAddress = header.next;
  let chunkLenSum = 0;
  do {
    const chunk = toRawStrChunk(buffer, chunkAddress);
    chunks.push(chunk);
    chunkAddress = chunk.next;
    chunkLenSum += chunk.len;
  } while (chunkAddress);
  const str = {
    len: header.len,
    chunks: chunks.map(({size, len, data}) => ({size, len, data})),
  };
  if (str.len !== chunkLenSum) {
    throw new Error(
      `String length mismatch: header length ${str.len}, ` +
        `sum of chunk lengths ${chunkLenSum}\n` +
        `Chunks: ${JSON.stringify(str.chunks)}`
    );
  }
  return str;
}

export function getWasmStr(memory: WebAssembly.Memory, address: number) {
  const str = getStr(memory, address);
  const result = str.chunks.map(({data}) => data).join('');
  if (result.length !== str.len) {
    throw new Error(
      `String length mismatch at address ${address}: ` +
        `expected ${str.len}, actual length is ${result.length}`
    );
  }
  return result;
}

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
      expectUsedChunks(wasmMemory, heapStart, 0);
      const str1 = strNew(0);
      expect(getStr(wasmMemory, str1)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      expectUsedChunks(wasmMemory, heapStart, 2);
      strDelete(str1);
      expectUsedChunks(wasmMemory, heapStart, 0);
      const str2 = strNew(0);
      expect(getStr(wasmMemory, str2)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      expectUsedChunks(wasmMemory, heapStart, 2);
      expect(str1).toStrictEqual(str2);
    });
    test('create and free string from raw string', async function () {
      const {strFromRaw, strDelete, wasmMemory} = await setup();
      expectUsedChunks(wasmMemory, heapStart, 0);
      setWasmString(wasmMemory, 4096, 'foo');
      const str1 = strFromRaw(4096);
      expectUsedChunks(wasmMemory, heapStart, 2);
      expect(getStr(wasmMemory, str1)).toStrictEqual({
        len: 3,
        chunks: [{size: 32, len: 3, data: 'foo'}],
      });
      strDelete(str1);
      expectUsedChunks(wasmMemory, heapStart, 0);

      const str2 = strFromRaw(4096 + 10); // nothing there
      expectUsedChunks(wasmMemory, heapStart, 2);
      expect(getStr(wasmMemory, str2)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      strDelete(str2);
      expectUsedChunks(wasmMemory, heapStart, 0);
    });
    test('merge strings', async function () {
      const {strNew, strFromRaw, strMerge, strDelete, strFlatten, wasmMemory} =
        await setup();

      const str0 = strNew(0);
      setWasmString(wasmMemory, 4096, 'foo');
      const str1 = strFromRaw(4096);
      setWasmString(wasmMemory, 4096 + 10, 'bar');
      const str2 = strFromRaw(4096 + 10);
      // Empty + non-empty
      strMerge(str0, str1);
      expect(getStr(wasmMemory, str0)).toStrictEqual({
        len: 3,
        chunks: [{size: 32, len: 3, data: 'foo'}],
      });
      // Non-empty + non-empty, but enough space in first string
      strMerge(str0, str2);
      expect(getStr(wasmMemory, str0)).toStrictEqual({
        len: 6,
        chunks: [{size: 32, len: 6, data: 'foobar'}],
      });
      expectUsedChunks(wasmMemory, heapStart, 2);

      const str3 = strNew(0);
      const str4 = strNew(0);
      // Empty + empty
      strMerge(str3, str4);
      expect(getStr(wasmMemory, str3)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      // Non-empty + empty
      strMerge(str0, str3);
      expect(getStr(wasmMemory, str0)).toStrictEqual({
        len: 6,
        chunks: [{size: 32, len: 6, data: 'foobar'}],
      });
      expectUsedChunks(wasmMemory, heapStart, 2);

      // Non-empty + large non-empt
      setWasmString(wasmMemory, 4096 + 20, 'a'.repeat(100));
      const str5 = strFromRaw(4096 + 20);
      expectUsedChunks(wasmMemory, heapStart, 4);
      strMerge(str0, str5);
      expect(getStr(wasmMemory, str0)).toStrictEqual({
        len: 106,
        chunks: [
          {size: 32, len: 6, data: 'foobar'},
          {size: 100, len: 100, data: 'a'.repeat(100)},
        ],
      });
      strFlatten(str0);
      expect(getStr(wasmMemory, str0)).toStrictEqual({
        len: 106,
        chunks: [{size: 108, len: 106, data: 'foobar' + 'a'.repeat(100)}],
      });
      expectUsedChunks(wasmMemory, heapStart, 2);

      // Lots of non-empty. Should never exceed 8 chunks.
      const str6 = strFromRaw(4096);
      for (let i = 0; i < 150; ++i) {
        const str7 = strFromRaw(4096);
        strMerge(str6, str7);
        expect(getStr(wasmMemory, str6).chunks.length).toBeLessThanOrEqual(8);
      }
    });
    test('push strings', async function () {
      const {strNew, strPushRaw, wasmMemory} = await setup();
      setWasmString(wasmMemory, 4096, 'A');
      const str1 = strNew(0);
      for (let i = 1; i <= 500; ++i) {
        strPushRaw(str1, 4096);
        expect(getStr(wasmMemory, str1)).toMatchObject({
          len: i,
        });
        expect(getWasmStr(wasmMemory, str1)).toStrictEqual('A'.repeat(i));
      }
    });
    test('compare strings', async function () {
      const {strNew, strPushRaw, strFromRaw, strCmp, strCmpRaw, wasmMemory} =
        await setup();
      // Basic strings.
      setWasmString(wasmMemory, 4096, 'foo');
      const str1 = strFromRaw(4096);
      const str2 = strFromRaw(4096);
      expect(strCmp(str1, str1)).toStrictEqual(0);
      expect(strCmpRaw(str1, 4096)).toStrictEqual(0);
      expect(strCmp(str1, str2)).toStrictEqual(0);

      setWasmString(wasmMemory, 4096, 'foobar');
      const str3 = strFromRaw(4096);
      expect(strCmp(str1, str3)).toBeLessThan(0);
      expect(strCmp(str3, str1)).toBeGreaterThan(0);

      const str4 = strFromRaw(4096 + 3); // "bar"
      expect(strCmp(str1, str4)).toBeGreaterThan(0);
      expect(strCmp(str4, str1)).toBeLessThan(0);

      // Empty strings.
      expect(strCmp(strNew(0), strNew(0))).toStrictEqual(0);

      // 1 chunk vs 2 chunks.
      setWasmString(wasmMemory, 4096 + 10, 'foobar' + 'a'.repeat(100));
      strPushRaw(str3, 4096 + 10 + 6);
      expect(getStr(wasmMemory, str3).chunks.length).toStrictEqual(2); // 'foobar' + 'a'.repeat(100)
      expect(strCmp(str3, strFromRaw(4096 + 10))).toStrictEqual(0);
    });
  });
}
