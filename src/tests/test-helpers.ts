import fs from 'fs-extra';
import path from 'path';
import {compileFilesWithRuntime, loadWasmModule} from '../quo-driver';

export interface MemChunk {
  address: number;
  size: number;
  next: number;
  prev: number;
  used: boolean;
}

function toMemChunk(buffer: Buffer, address: number): MemChunk {
  return {
    address,
    size: buffer.readUInt32LE(address + 0),
    next: buffer.readUInt32LE(address + 4),
    prev: buffer.readUInt32LE(address + 8),
    used: Boolean(buffer.readUInt8(address + 12)),
  };
}

export function getMemChunks(
  memory: WebAssembly.Memory,
  heapStart: number
): Array<MemChunk> {
  const buffer = Buffer.from(memory.buffer);
  const head = toMemChunk(buffer, heapStart);
  const chunks = [head];
  let chunk = head;
  while (chunk.next !== 0) {
    chunk = toMemChunk(buffer, chunk.next);
    chunks.push(chunk);
  }
  return chunks;
}

function getUsedMemChunks(memory: WebAssembly.Memory, heapStart: number) {
  return getMemChunks(memory, heapStart).filter((chunk) => chunk.used);
}

export function expectUsedMemChunks(
  memory: WebAssembly.Memory,
  heapStart: number,
  num: number
) {
  expect(getUsedMemChunks(memory, heapStart)).toHaveLength(num);
}

export interface StrChunk {
  size: number;
  len: number;
  data: string;
}

export interface StrObject {
  len: number;
  chunks: Array<StrChunk>;
}

const strChunkHeaderSize = 12;

export function toRawStrChunk(
  buffer: Buffer,
  address: number
): [StrChunk, number] {
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
  return [
    {
      size: buffer.readUInt32LE(address + 0),
      len: chunkLen,
      data,
    },
    buffer.readUInt32LE(address + 8),
  ];
}

export function getStrObject(
  memory: WebAssembly.Memory,
  address: number
): StrObject {
  const buffer = Buffer.from(memory.buffer);
  const header = {
    len: buffer.readUInt32LE(address + 0),
    next: buffer.readUInt32LE(address + 4),
  };
  const chunks = [];
  let chunkAddress = header.next;
  let chunkLenSum = 0;
  do {
    const [chunk, next] = toRawStrChunk(buffer, chunkAddress);
    chunks.push(chunk);
    chunkAddress = next;
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

export function getStr(memory: WebAssembly.Memory, address: number) {
  const str = getStrObject(memory, address);
  const result = str.chunks.map(({data}) => data).join('');
  if (result.length !== str.len) {
    throw new Error(
      `String length mismatch at address ${address}: ` +
        `expected ${str.len}, actual length is ${result.length}`
    );
  }
  return result;
}

export async function compileAndLoadAsModule(
  stage: string,
  input: string,
  name: string
) {
  const [inputFilePath, outputFilePath] = ['quo', 'wasm'].map((ext) =>
    path.join(__dirname, 'testdata', `tmp-${name}.stage${stage}.${ext}`)
  );
  await fs.writeFile(inputFilePath, input);
  const {watOutputFilePath, wasmOutputFilePath} = await compileFilesWithRuntime(
    stage,
    [inputFilePath],
    outputFilePath
  );
  const {wasmModule, wasmMemory, fns} =
    await loadWasmModule(wasmOutputFilePath);
  const deleteTempFiles = async () => {
    await Promise.all([
      fs.rm(inputFilePath),
      fs.rm(watOutputFilePath),
      fs.rm(wasmOutputFilePath),
    ]);
  };
  return {
    wasmModule,
    wasmMemory,
    fns,
    deleteTempFiles,
  };
}
