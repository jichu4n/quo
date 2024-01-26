import {setupWasmModule, wrapWebAssemblyFn, setWasmString} from '../quo-driver';
import {getChunks} from './memory.test';

const stages = ['1a'];

const memoryStart = 32;
const memoryEnd = 1024;
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
  return {
    address,
    size: buffer.readUInt32LE(address + 0),
    len: chunkLen,
    next: buffer.readUInt32LE(address + 8),
    data: buffer.toString('utf8', dataStart, dataEnd),
  };
}

function getStr(memory: WebAssembly.Memory, address: number): Str {
  const buffer = Buffer.from(memory.buffer);
  const header = toRawStrHeader(buffer, address);
  const chunks = [];
  let chunkAddress = header.next;
  do {
    const chunk = toRawStrChunk(buffer, chunkAddress);
    chunks.push(chunk);
    chunkAddress = chunk.next;
  } while (chunkAddress);
  return {
    len: header.len,
    chunks: chunks.map(({size, len, data}) => ({size, len, data})),
  };
}

function expectUsedChunks(memory: WebAssembly.Memory, num: number) {
  expect(getChunks(memory).filter((chunk) => chunk.used)).toHaveLength(num);
}

for (const stage of stages) {
  const setup = async () => {
    const {wasmModule, wasmMemory} = await setupWasmModule(stage);
    const memoryInit = wrapWebAssemblyFn(
      wasmMemory,
      wasmModule.instance.exports.memoryInit as CallableFunction
    );
    const strNew = wrapWebAssemblyFn(
      wasmMemory,
      wasmModule.instance.exports.strNew as CallableFunction
    );
    const strDelete = wrapWebAssemblyFn(
      wasmMemory,
      wasmModule.instance.exports.strDelete as CallableFunction
    );
    const strFromRaw = wrapWebAssemblyFn(
      wasmMemory,
      wasmModule.instance.exports.strFromRaw as CallableFunction
    );
    const strToRaw = wrapWebAssemblyFn(
      wasmMemory,
      wasmModule.instance.exports.strToRaw as CallableFunction
    );
    const strMerge = wrapWebAssemblyFn(
      wasmMemory,
      wasmModule.instance.exports.strMerge as CallableFunction
    );
    const strAppendRaw = wrapWebAssemblyFn(
      wasmMemory,
      wasmModule.instance.exports.strAppendRaw as CallableFunction
    );

    memoryInit(memoryStart, memoryEnd);
    return {
      strNew,
      strDelete,
      strFromRaw,
      strToRaw,
      strMerge,
      strAppendRaw,
      wasmMemory,
    };
  };

  describe(`stage ${stage} strings`, () => {
    test('create and free empty string', async function () {
      const {strNew, strDelete, wasmMemory} = await setup();
      expectUsedChunks(wasmMemory, 0);
      const str1 = strNew(0);
      expect(getStr(wasmMemory, str1)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      expectUsedChunks(wasmMemory, 2);
      strDelete(str1);
      expectUsedChunks(wasmMemory, 0);
      const str2 = strNew(0);
      expect(getStr(wasmMemory, str2)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      expectUsedChunks(wasmMemory, 2);
      expect(str1).toStrictEqual(str2);
    });
    test('create and free string from raw string', async function () {
      const {strFromRaw, strDelete, wasmMemory} = await setup();
      expectUsedChunks(wasmMemory, 0);
      setWasmString(wasmMemory, 4096, 'foo');
      const str1 = strFromRaw(4096);
      expectUsedChunks(wasmMemory, 2);
      expect(getStr(wasmMemory, str1)).toStrictEqual({
        len: 3,
        chunks: [{size: 32, len: 3, data: 'foo'}],
      });
      strDelete(str1);
      expectUsedChunks(wasmMemory, 0);

      const str2 = strFromRaw(4096 + 10); // nothing there
      expectUsedChunks(wasmMemory, 2);
      expect(getStr(wasmMemory, str2)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      strDelete(str2);
      expectUsedChunks(wasmMemory, 0);
    });
    test('merge strings', async function () {
      const {strNew, strFromRaw, strMerge, strDelete, wasmMemory} =
        await setup();
      const str0 = strNew(0);
      setWasmString(wasmMemory, 4096, 'foo');
      const str1 = strFromRaw(4096);
      setWasmString(wasmMemory, 4096 + 10, 'bar');
      const str2 = strFromRaw(4096 + 10);
      strMerge(str0, str1);
      expect(getStr(wasmMemory, str0)).toStrictEqual({
        len: 3,
        chunks: [{size: 32, len: 3, data: 'foo'}],
      });
      strMerge(str0, str2);
      expect(getStr(wasmMemory, str0)).toStrictEqual({
        len: 6,
        chunks: [{size: 32, len: 6, data: 'foobar'}],
      });
      expectUsedChunks(wasmMemory, 2);
      const str3 = strNew(0);
      const str4 = strNew(0);
      strMerge(str3, str4);
      expect(getStr(wasmMemory, str3)).toStrictEqual({
        len: 0,
        chunks: [{size: 32, len: 0, data: ''}],
      });
      strMerge(str0, str3);
      expect(getStr(wasmMemory, str0)).toStrictEqual({
        len: 6,
        chunks: [{size: 32, len: 6, data: 'foobar'}],
      });
      expectUsedChunks(wasmMemory, 2);
    });
  });
}
