(module
  ;; Main memory - 16MB.
  (import "env" "memory" (memory 256))
  (import "env" "tag" (tag $error (param i32)))
  (import "debug" "puts" (func $puts (param i32)))

  ;; Memory allocation.
  ;;
  ;; We simply increment the memory offset by the size of the allocation, and
  ;; never free any allocated memory.
  (global $memoryOffset (mut i32) (i32.const 0))
  (func $alloc (param $size i32) (result i32)
    (local $ptr i32)
    (local.set $ptr (global.get $memoryOffset))
    (global.set
      $memoryOffset
      (i32.add (global.get $memoryOffset) (local.get $size)))
    (if (i32.ge_u (global.get $memoryOffset) (i32.const 15728640))
      (then (throw $error (i32.const 15728640)))
    )
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

  ;; Tokenizer.
  ;;
  ;; Token types:
  ;;   00 - EOF
  ;;   01 - Number literal
  ;;   02 - String literal
  ;;   03 - Identifier
  ;;   04 - fn
  ;;   05 - let
  ;;   06 - if
  ;;   07 - else
  ;;   07 - while
  ;;   09 - return
  ;; Single letter operators and symbol are their ASCII value. Double letter
  ;; operators are 128 + the ASCII value of the first letter.
  (global $input (mut i32) (i32.const 0))
  (global $inputPtr (mut i32) (i32.const 0))
  (func $nextToken (param $tokenValuePtr i32) (result i32)
    (local $c i32)
    (local $numberValue i32)
    (local $stringValuePtr i32)
    (call $skipWhitespaceAndComments)
    (local.set $c (i32.load8_u (global.get $inputPtr)))

    ;; EOF
    (if (i32.eqz (local.get $c))
      (then (return (i32.const 0)))
    )

    ;; Number literal
    (if (call $isdigit (local.get $c))
      (then
        (local.set $numberValue (i32.const 0))
        (loop $loop
          (local.set $numberValue
            (i32.add
              (i32.mul (local.get $numberValue) (i32.const 10))
              (i32.sub (local.get $c) (i32.const 48))
            )
          )
          (call $inputPtrInc)
          (local.set $c (i32.load8_u (global.get $inputPtr)))
          (br_if $loop (call $isdigit (local.get $c)))
        )
        (i32.store (local.get $tokenValuePtr) (local.get $numberValue))
        (return (i32.const 1))
      )
    )

    ;; String literal
    (if (i32.eq (local.get $c) (i32.const 34)) ;; "
      (then
        (local.set $stringValuePtr (local.get $tokenValuePtr))
        (block $loop_end
          (loop $loop
            (call $inputPtrInc)
            (local.set $c (i32.load8_u (global.get $inputPtr)))
            (if (i32.eqz (local.get $c))
              (then (throw $error (i32.const 15728656)))
            )
            (br_if $loop_end (i32.eq (local.get $c) (i32.const 34))) ;; "
            (i32.store8 (local.get $stringValuePtr) (local.get $c))
            (local.set $stringValuePtr (i32.add (local.get $stringValuePtr) (i32.const 1)))
            (br $loop)
          )
        )
        (i32.store8 (local.get $stringValuePtr) (i32.const 0))
        (call $inputPtrInc)
        (return (i32.const 2))
      )
    )

    ;; Identifier or keyword.
    (if (call $isident (local.get $c))
      (then
        (local.set $stringValuePtr (local.get $tokenValuePtr))
        (loop $loop
          (i32.store8 (local.get $stringValuePtr) (local.get $c))
          (local.set $stringValuePtr (i32.add (local.get $stringValuePtr) (i32.const 1)))
          (call $inputPtrInc)
          (local.set $c (i32.load8_u (global.get $inputPtr)))
          (if (call $isident (local.get $c))
            (then (br $loop))
          )
        )
        (i32.store8 (local.get $stringValuePtr) (i32.const 0))
        (return
          (i32.add
            (i32.const 4)
            (call $strfind (local.get $tokenValuePtr) (i32.const 15728896))
          )
        )
      )
    )

    ;; Operators and symbols.
    (if (call $strchr (i32.const 15729536) (local.get $c))
      (then
        (call $inputPtrInc)
        (i32.store8 (local.get $tokenValuePtr) (local.get $c))
        (i32.store8
          (i32.add (local.get $tokenValuePtr) (i32.const 1))
          (i32.load8_u (global.get $inputPtr))
        )
        (i32.store8
          (i32.add (local.get $tokenValuePtr) (i32.const 2))
          (i32.const 0)
        )
        (if
          (i32.ge_s
            (call $strfind (local.get $tokenValuePtr) (i32.const 15729408))
            (i32.const 0)
          )
          (then
            (call $inputPtrInc)
            (return (i32.add (i32.const 128) (local.get $c)))
          )
          (else
            (i32.store8
              (i32.add (local.get $tokenValuePtr) (i32.const 1))
              (i32.const 0)
            )
            (return (local.get $c))
          )
        )
      )
    )

    (i32.store8 (local.get $tokenValuePtr) (local.get $c))
    (i32.store8 (i32.add (local.get $tokenValuePtr) (i32.const 1)) (i32.const 0))
    (call $strcat (i32.const 15728768) (local.get $tokenValuePtr))
    (throw $error (i32.const 15728768))
  )
  (export "nextToken" (func $nextToken))
  (func $skipWhitespaceAndComments
    (local $c i32)
    (block $loop_end
      (loop $loop
        (local.set $c (i32.load8_u (global.get $inputPtr)))
        (br_if $loop_end (i32.eqz (local.get $c)))
        (if (call $isspace (local.get $c))
          (then
            (call $inputPtrInc)
            (br $loop)
          )
        )
        (if
          (i32.and
            (i32.eq (local.get $c) (i32.const 47)) ;; /
            (i32.eq
              (i32.load8_u (i32.add (global.get $inputPtr) (i32.const 1)))
              (i32.const 47)
            ) ;; /
          )
          (then
            (global.set $inputPtr (i32.add (global.get $inputPtr) (i32.const 2)))
            (block $comment_loop_end
              (loop $comment_loop
                (local.set $c (i32.load8_u (global.get $inputPtr)))
                (br_if $loop_end (i32.eqz (local.get $c)))
                (call $inputPtrInc)
                (br_if $comment_loop_end (i32.eq (local.get $c) (i32.const 10))) ;; LF
                (br $comment_loop)
              )
            )
            (br $loop)
          )
        )
      )
    )
  )
  (func $inputPtrInc
    (global.set $inputPtr (i32.add (global.get $inputPtr) (i32.const 1)))
  )

  ;; String functions.
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
  (func $isspace (param $c i32) (result i32)
    (block $true
      (br_if $true (i32.eq (local.get $c) (i32.const 32))) ;; space
      (br_if $true (i32.eq (local.get $c) (i32.const 9)))  ;; tab
      (br_if $true (i32.eq (local.get $c) (i32.const 13))) ;; CR
      (br_if $true (i32.eq (local.get $c) (i32.const 10))) ;; LF
      (return (i32.const 0))
    )
    (i32.const 1)
  )
  (func $isdigit (param $c i32) (result i32)
    (if (i32.and
          (i32.ge_u (local.get $c) (i32.const 48)) ;; 0
          (i32.le_u (local.get $c) (i32.const 57)) ;; 9
        )
      (then (return (i32.const 1)))
    )
    (i32.const 0)
  )
  (func $isalpha (param $c i32) (result i32)
    (if
      (i32.or
        (i32.and
          (i32.ge_u (local.get $c) (i32.const 65)) ;; A
          (i32.le_u (local.get $c) (i32.const 90)) ;; Z
        )
        (i32.and
          (i32.ge_u (local.get $c) (i32.const 97)) ;; a
          (i32.le_u (local.get $c) (i32.const 122)) ;; z
        )
      )
      (then (return (i32.const 1)))
    )
    (i32.const 0)
  )
  (func $isalnum (param $c i32) (result i32)
    (if (i32.or
          (call $isdigit (local.get $c))
          (call $isalpha (local.get $c))
        )
      (then (return (i32.const 1)))
    )
    (i32.const 0)
  )
  (func $isident (param $c i32) (result i32)
    (if
      (i32.or
        (call $isalnum (local.get $c))
        (i32.or
          (i32.eq (local.get $c) (i32.const 95)) ;; _
          (i32.eq (local.get $c) (i32.const 36)) ;; $
        )
      )
      (then (return (i32.const 1)))
    )
    (i32.const 0)
  )
  (func $strcmp (param $str1 i32) (param $str2 i32) (result i32)
    (local $c1 i32)
    (local $c2 i32)
    (block $loop_end
      (loop $loop
        (local.set $c1 (i32.load8_u (local.get $str1)))
        (local.set $c2 (i32.load8_u (local.get $str2)))
        (br_if $loop_end
          (i32.or
            (i32.or (i32.eqz (local.get $c1)) (i32.eqz (local.get $c2)))
            (i32.ne (local.get $c1) (local.get $c2))
          )
        )
        (local.set $str1 (i32.add (local.get $str1) (i32.const 1)))
        (local.set $str2 (i32.add (local.get $str2) (i32.const 1)))
        (br $loop)
      )
    )
    (i32.sub (local.get $c1) (local.get $c2))
  )
  (func $strfind (param $str i32) (param $strlist i32) (result i32)
    (local $index i32)
    (local $c i32)
    (local.set $index (i32.const 0))
    (block $loop_end
      (loop $loop
        (br_if $loop_end (i32.eqz (i32.load8_u (local.get $strlist))))
        (if (i32.eqz (call $strcmp (local.get $str) (local.get $strlist)))
          (then (return (local.get $index)))
        )
        (local.set $strlist
          (i32.add
            (i32.add (local.get $strlist) (call $strlen (local.get $strlist)))
            (i32.const 1)
          )
        )
        (local.set $index (i32.add (local.get $index) (i32.const 1)))
        (br $loop)
      )
    )
    (i32.const -1)
  )
  (func $strchr (param $str i32) (param $c i32) (result i32)
    (local $c2 i32)
    (loop $loop
      (local.set $c2 (i32.load8_u (local.get $str)))
      (if (i32.eqz (local.get $c2))
        (then (return (i32.const 0)))
      )
      (if $loop_end (i32.eq (local.get $c2) (local.get $c))
        (then (return (local.get $str)))
      )
      (local.set $str (i32.add (local.get $str) (i32.const 1)))
      (br $loop)
    )
    unreachable
  )
  (func $strcat (param $str1 i32) (param $str2 i32)
    (local $len1 i32)
    (local $len2 i32)
    (local.set $len1 (call $strlen (local.get $str1)))
    (local.set $len2 (call $strlen (local.get $str2)))
    (memory.copy
      (i32.add (local.get $str1) (local.get $len1))
      (local.get $str2)
      (i32.add (local.get $len2) (i32.const 1))
    )
  )

  ;; Entry point.
  (func (export "main") (param $input i32) (result i32)
    (local $output i32)

    (call $init (local.get $input))
    (local.set $output (call $alloc (i32.const 1048576)))

    (call $skipWhitespaceAndComments)
    (memory.copy
        (local.get $output)
        (global.get $inputPtr)
        (i32.add (call $strlen (global.get $inputPtr)) (i32.const 1)))

    (local.get $output)
  )
  (func $init (param $input i32)
    ;; Set initial memory offset to after input string.
    (global.set
        $memoryOffset
        (i32.add (call $strlen (local.get $input)) (i32.const 1)))
    ;; Set up tokenizer state
    (global.set $input (local.get $input))
    (global.set $inputPtr (local.get $input))
  )
  (export "init" (func $init))

  ;; Data section starts at 15MB.
  ;; OOM error message
  (data (i32.const 15728640)
    "Out of memory\00"
  )
  (data (i32.const 15728656)
    "Unterminated string literal\00"
  )
  ;; Invalid token error message
  (data (i32.const 15728768)
    "Unrecognized character: \00"
  )
  ;; Keywords
  (data (i32.const 15728896)
    "fn\00"
    "let\00"
    "if\00"
    "else\00"
    "while\00"
    "return\00"
    "\00"
  )
  ;; 2-letter operators
  (data (i32.const 15729408)
    "==\00"
    "!=\00"
    "<=\00"
    ">=\00"
    "&&\00"
    "||\00"
    "\00"
  )
  ;; 1-letter operators and symbols
  (data (i32.const 15729536)
    "(){}[]<>=+-*/%!&|.,;:^\00"
  )
)