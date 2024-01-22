import fs from 'fs-extra';
import path from 'path';

const input = 'hello, world\0';

if (require.main === module) {
  (async () => {
    const wasmFile = await fs.readFile(path.join(__dirname, 'main.wasm'));
    const wasmMemory = new WebAssembly.Memory({initial: 256});
    const wasmModule = await WebAssembly.instantiate(wasmFile, {
      env: {memory: wasmMemory},
    });
    const main = wasmModule.instance.exports.main as CallableFunction;

    new TextEncoder().encodeInto(input, new Uint8Array(wasmMemory.buffer));
    const $output = main(0);
    const outputLen = new Uint8Array(wasmMemory.buffer, $output).findIndex(
      (byte) => byte === 0
    );
    const output = new TextDecoder().decode(
      new Uint8Array(wasmMemory.buffer, $output, outputLen)
    );
    console.log(output);
  })();
}
