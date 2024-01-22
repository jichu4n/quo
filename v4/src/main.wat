(module
  ;; Main memory - 16MB.
  (import "env" "memory" (memory 256))

  ;; Memory allocation.
  ;;
  ;; We simply increment the memory offset by the size of the allocation, and
  ;; never free any allocated memory.
  (global $memoryOffset (mut i32) (i32.const 0))
  (func $alloc (param $size i32) (result i32)
    (local $ptr i32)
    (local.set $ptr (global.get $memoryOffset))
    (global.set $memoryOffset (i32.add (global.get $memoryOffset) (local.get $size)))
    (local.get $ptr)
  )
  (func $realloc (param $ptr i32) (param $oldSize i32) (param $newSize i32) (result i32)
    (local $newPtr i32)
    (if (i32.le_u (local.get $newSize) (local.get $oldSize))
      (then (return (local.get $ptr)))
    )
    (local.set $newPtr (call $alloc (local.get $newSize)))
    (memory.copy (local.get $newPtr) (local.get $ptr) (local.get $oldSize))
    (local.get $newPtr)
  )

  (func $strlen (param $str i32) (result i32)
    (local $len i32)
    (block $loop_end
      (loop $loop
        (br_if $loop_end (i32.eqz (i32.load8_u (local.get $str))))
        (local.set $str (i32.add (local.get $str) (i32.const 1)))
        (local.set $len (i32.add (local.get $len) (i32.const 1)))
        (br $loop)
      )
    )
    (local.get $len)
  )

  ;; Entry point.
  (func (export "main") (param $input i32) (result i32)
    (local $output i32)

    ;; Set initial memory offset to after input string.
    (global.set
        $memoryOffset
        (i32.add (call $strlen (local.get $input)) (i32.const 1)))

    ;; Set up output string
    (local.set $output (call $alloc (i32.const 4096)))
    (memory.copy
        (local.get $output)
        (i32.const 15728640)
        (i32.add (call $strlen (i32.const 15728640)) (i32.const 1)))

    (local.get $output)
  )

  ;; Data section starts at 15MB.
  (data (i32.const 15728640)
    "Hello, world!\00"
  )
)