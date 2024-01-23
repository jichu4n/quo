// WebAssembly types for exception handling are currently missing from
// TypeScript's built-in types (lib.dom.d.ts).
declare namespace WebAssembly {
  var Tag: {
    prototype: Tag;
    new (options: {parameters: Array<string>}): Tag;
    (options: {parameters: Array<string>}): Tag;
  };
  type Tag = any;
  var Exception: {
    prototype: Exception;
    new (): Exception;
    (): Exception;
  };
  interface Exception {
    is(tag: Tag): boolean;
    getArg(tag: Tag, index: number): number;
  }
}
