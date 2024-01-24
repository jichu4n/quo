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

  ;; Symbol tables used by parser / code generator.
  (global $globalVars (mut i32) (i32.const 0))
  (global $localVars (mut i32) (i32.const 0))
  (global $fns (mut i32) (i32.const 0))
  (func $addLocalVar (param $name i32)
    (call $strlistAppend (global.get $localVars) (i32.const 8192) (local.get $name))
  )
  (func $isLocalVar (param $name i32) (result i32)
    (i32.ge_s (call $strlistFind (global.get $localVars) (local.get $name)) (i32.const 0))
  )
  (func $addGlobalVar (param $name i32)
    (call $strlistAppend (global.get $globalVars) (i32.const 8192) (local.get $name))
  )
  (func $isGlobalVar (param $name i32) (result i32)
    (i32.ge_s (call $strlistFind (global.get $globalVars) (local.get $name)) (i32.const 0))
  )

  ;; Single pass parser / code generator.
  (func $compileExpr0 (result i32)
    (local $token i32)
    (local $tokenValuePtr i32)
    (local $outputPtr i32)
    (local $origInputPtr i32)
    (local $token2 i32)
    (local $tokenValuePtr2 i32)
    (local.set $tokenValuePtr (call $alloc (i32.const 128)))
    (local.set $token (call $nextToken (local.get $tokenValuePtr)))

    ;; Number literal.
    (if (i32.eq (local.get $token) (i32.const 1))
      (then
        (local.set $outputPtr (call $alloc (i32.const 128)))
        (call $strcpy (local.get $outputPtr) (i32.const 128) (i32.const 15729796))
        (call $strcat (local.get $outputPtr) (i32.const 128) (local.get $tokenValuePtr))
        (call $strcat (local.get $outputPtr) (i32.const 128) (i32.const 15729792))
        (return (local.get $outputPtr))
      )
    )

    ;; Parenthesis.
    (if (i32.eq (local.get $token) (i32.const 40)) ;; (
      (then
        (local.set $outputPtr (call $compileExpr))
        (call $expectToken (i32.const 41) (local.get $tokenValuePtr)) ;; )
        (return (local.get $outputPtr))
      )
    )

    ;; Identifier or function call
    (if (i32.eq (local.get $token) (i32.const 3))
      (then
        (local.set $origInputPtr (global.get $inputPtr))
        (local.set $tokenValuePtr2 (call $alloc (i32.const 128)))
        (local.set $token2 (call $nextToken (local.get $tokenValuePtr2)))
        (if (i32.eq (local.get $token2) (i32.const 40)) ;; (
          (then
            ;; Function call
            (local.set $outputPtr (call $alloc (i32.const 1024)))
            (call $strcpy (local.get $outputPtr) (i32.const 1024) (i32.const 15730372))
            (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $tokenValuePtr))
            (block $loop_end
              (local.set $origInputPtr (global.get $inputPtr))
              (local.set $token2 (call $nextToken (local.get $tokenValuePtr2)))
              (br_if $loop_end (i32.eq (local.get $token2) (i32.const 41)))
              (global.set $inputPtr (local.get $origInputPtr))
              (loop $loop
                (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729794))
                (call $strcat (local.get $outputPtr) (i32.const 1024) (call $compileExpr))
                (local.set $token2 (call $nextToken (local.get $tokenValuePtr2)))
                (br_if $loop (i32.eq (local.get $token2) (i32.const 44))) ;; ,
                (br_if $loop_end (i32.eq (local.get $token2) (i32.const 41)))
                (throw $error (i32.const 15730404))
              )
            )
            (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729792))
            (return (local.get $outputPtr))
          )
          (else
            ;; Identifier
            (global.set $inputPtr (local.get $origInputPtr))
            (if (call $isLocalVar (local.get $tokenValuePtr))
              (then
                (local.set $outputPtr (call $alloc (i32.const 128)))
                (call $strcpy (local.get $outputPtr) (i32.const 128) (i32.const 15730276))
                (call $strcat (local.get $outputPtr) (i32.const 128) (local.get $tokenValuePtr))
                (call $strcat (local.get $outputPtr) (i32.const 128) (i32.const 15729792))
                (return (local.get $outputPtr))
              )
            )
            (if (call $isGlobalVar (local.get $tokenValuePtr))
              (then
                (local.set $outputPtr (call $alloc (i32.const 128)))
                (call $strcpy (local.get $outputPtr) (i32.const 128) (i32.const 15730308))
                (call $strcat (local.get $outputPtr) (i32.const 128) (local.get $tokenValuePtr))
                (call $strcat (local.get $outputPtr) (i32.const 128) (i32.const 15729792))
                (return (local.get $outputPtr))
              )
            )
            (throw $error (i32.const 15730340))
          )
        )
      )
    )

    (throw $error (i32.const 15728688))
    ;; TODO: String literal, function call
  )
  (func $compileExpr1 (result i32)
    (local $token i32)
    (local $tokenValuePtr i32)
    (local $outputPtr i32)
    (local $rightOutputPtr i32)
    (local $origInputPtr i32)
    (local $outputPrefixPtr i32)

    (local.set $origInputPtr (global.get $inputPtr))
    (local.set $tokenValuePtr (call $alloc (i32.const 128)))
    (local.set $token (call $nextToken (local.get $tokenValuePtr)))
    (block $if_end
      (if (i32.eq (local.get $token) (i32.const 45)) ;; -
        (then (local.set $outputPrefixPtr (i32.const 15730212)) (br $if_end))
      )
      (if (i32.eq (local.get $token) (i32.const 33)) ;; !
        (then (local.set $outputPrefixPtr (i32.const 15730244)) (br $if_end))
      )
      (global.set $inputPtr (local.get $origInputPtr))
      (return (call $compileExpr0))
    )
    (local.set $rightOutputPtr (call $compileExpr0))
    (local.set $outputPtr (call $alloc (i32.const 1024)))
    (call $strcpy (local.get $outputPtr) (i32.const 1024) (local.get $outputPrefixPtr))
    (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $rightOutputPtr))
    (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729792))
    (local.get $outputPtr)
  )
  (func $compileExpr2 (result i32)
    (local $token i32)
    (local $tokenValuePtr i32)
    (local $outputPtr i32)
    (local $leftOutputPtr i32)
    (local $rightOutputPtr i32)
    (local $origInputPtr i32)
    (local $outputPrefixPtr i32)

    (local.set $leftOutputPtr (call $compileExpr1))
    (local.set $tokenValuePtr (call $alloc (i32.const 128)))
    (block $loop_end
      (loop $loop
        (local.set $origInputPtr (global.get $inputPtr))
        (local.set $token (call $nextToken (local.get $tokenValuePtr)))
        (block $if_end
          (if (i32.eq (local.get $token) (i32.const 42)) ;; *
            (then (local.set $outputPrefixPtr (i32.const 15729828)) (br $if_end))
          )
          (if (i32.eq (local.get $token) (i32.const 47)) ;; /
            (then (local.set $outputPrefixPtr (i32.const 15729860)) (br $if_end))
          )
          (global.set $inputPtr (local.get $origInputPtr))
          (br $loop_end)
        )
        (local.set $rightOutputPtr (call $compileExpr1))
        (local.set $outputPtr (call $alloc (i32.const 1024)))
        (call $strcpy (local.get $outputPtr) (i32.const 1024) (local.get $outputPrefixPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $leftOutputPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729794))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $rightOutputPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729792))
        (local.set $leftOutputPtr (local.get $outputPtr))
        (br $loop)
      )
    )
    (local.get $leftOutputPtr)
  )
  (func $compileExpr3 (result i32)
    (local $token i32)
    (local $tokenValuePtr i32)
    (local $outputPtr i32)
    (local $leftOutputPtr i32)
    (local $rightOutputPtr i32)
    (local $origInputPtr i32)
    (local $outputPrefixPtr i32)

    (local.set $leftOutputPtr (call $compileExpr2))
    (local.set $tokenValuePtr (call $alloc (i32.const 128)))
    (block $loop_end
      (loop $loop
        (local.set $origInputPtr (global.get $inputPtr))
        (local.set $token (call $nextToken (local.get $tokenValuePtr)))
        (block $if_end
          (if (i32.eq (local.get $token) (i32.const 43)) ;; +
            (then (local.set $outputPrefixPtr (i32.const 15729892)) (br $if_end))
          )
          (if (i32.eq (local.get $token) (i32.const 45)) ;; -
            (then (local.set $outputPrefixPtr (i32.const 15729924)) (br $if_end))
          )
          (global.set $inputPtr (local.get $origInputPtr))
          (br $loop_end)
        )
        (local.set $rightOutputPtr (call $compileExpr2))
        (local.set $outputPtr (call $alloc (i32.const 1024)))
        (call $strcpy (local.get $outputPtr) (i32.const 1024) (local.get $outputPrefixPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $leftOutputPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729794))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $rightOutputPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729792))
        (local.set $leftOutputPtr (local.get $outputPtr))
        (br $loop)
      )
    )
    (local.get $leftOutputPtr)
  )
  (func $compileExpr4 (result i32)
    (local $token i32)
    (local $tokenValuePtr i32)
    (local $outputPtr i32)
    (local $leftOutputPtr i32)
    (local $rightOutputPtr i32)
    (local $origInputPtr i32)
    (local $outputPrefixPtr i32)

    (local.set $leftOutputPtr (call $compileExpr3))
    (local.set $tokenValuePtr (call $alloc (i32.const 128)))
    (local.set $origInputPtr (global.get $inputPtr))
    (local.set $token (call $nextToken (local.get $tokenValuePtr)))
    (block $loop_end
      (block $if_end
        (if (i32.eq (local.get $token) (i32.const 189)) ;; ==
          (then (local.set $outputPrefixPtr (i32.const 15729956)) (br $if_end))
        )
        (if (i32.eq (local.get $token) (i32.const 161)) ;; !=
          (then (local.set $outputPrefixPtr (i32.const 15729988)) (br $if_end))
        )
        (if (i32.eq (local.get $token) (i32.const 62)) ;; >
          (then (local.set $outputPrefixPtr (i32.const 15730020)) (br $if_end))
        )
        (if (i32.eq (local.get $token) (i32.const 60)) ;; <
          (then (local.set $outputPrefixPtr (i32.const 15730052)) (br $if_end))
        )
        (if (i32.eq (local.get $token) (i32.const 190)) ;; >=
          (then (local.set $outputPrefixPtr (i32.const 15730084)) (br $if_end))
        )
        (if (i32.eq (local.get $token) (i32.const 188)) ;; <=
          (then (local.set $outputPrefixPtr (i32.const 15730116)) (br $if_end))
        )
        (global.set $inputPtr (local.get $origInputPtr))
        (br $loop_end)
      )
      (local.set $rightOutputPtr (call $compileExpr2))
      (local.set $outputPtr (call $alloc (i32.const 1024)))
      (call $strcpy (local.get $outputPtr) (i32.const 1024) (local.get $outputPrefixPtr))
      (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $leftOutputPtr))
      (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729794))
      (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $rightOutputPtr))
      (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729792))
      (local.set $leftOutputPtr (local.get $outputPtr))
    )
    (local.get $leftOutputPtr)
  )
  (func $compileExpr5 (result i32)
    (local $token i32)
    (local $tokenValuePtr i32)
    (local $outputPtr i32)
    (local $leftOutputPtr i32)
    (local $rightOutputPtr i32)
    (local $origInputPtr i32)
    (local $outputPrefixPtr i32)

    (local.set $leftOutputPtr (call $compileExpr4))
    (local.set $tokenValuePtr (call $alloc (i32.const 128)))
    (block $loop_end
      (loop $loop
        (local.set $origInputPtr (global.get $inputPtr))
        (local.set $token (call $nextToken (local.get $tokenValuePtr)))
        (block $if_end
          (if (i32.eq (local.get $token) (i32.const 166)) ;; &&
            (then (local.set $outputPrefixPtr (i32.const 15730148)) (br $if_end))
          )
          (global.set $inputPtr (local.get $origInputPtr))
          (br $loop_end)
        )
        (local.set $rightOutputPtr (call $compileExpr4))
        (local.set $outputPtr (call $alloc (i32.const 1024)))
        (call $strcpy (local.get $outputPtr) (i32.const 1024) (local.get $outputPrefixPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $leftOutputPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729794))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $rightOutputPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729792))
        (local.set $leftOutputPtr (local.get $outputPtr))
        (br $loop)
      )
    )
    (local.get $leftOutputPtr)
  )
  (func $compileExpr6 (result i32)
    (local $token i32)
    (local $tokenValuePtr i32)
    (local $outputPtr i32)
    (local $leftOutputPtr i32)
    (local $rightOutputPtr i32)
    (local $origInputPtr i32)
    (local $outputPrefixPtr i32)

    (local.set $leftOutputPtr (call $compileExpr5))
    (local.set $tokenValuePtr (call $alloc (i32.const 128)))
    (block $loop_end
      (loop $loop
        (local.set $origInputPtr (global.get $inputPtr))
        (local.set $token (call $nextToken (local.get $tokenValuePtr)))
        (block $if_end
          (if (i32.eq (local.get $token) (i32.const 252)) ;; ||
            (then (local.set $outputPrefixPtr (i32.const 15730180)) (br $if_end))
          )
          (global.set $inputPtr (local.get $origInputPtr))
          (br $loop_end)
        )
        (local.set $rightOutputPtr (call $compileExpr5))
        (local.set $outputPtr (call $alloc (i32.const 1024)))
        (call $strcpy (local.get $outputPtr) (i32.const 1024) (local.get $outputPrefixPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $leftOutputPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729794))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $rightOutputPtr))
        (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729792))
        (local.set $leftOutputPtr (local.get $outputPtr))
        (br $loop)
      )
    )
    (local.get $leftOutputPtr)
  )
  (func $expectToken (param $expectedToken i32) (param $tokenValuePtr i32)
    (if (i32.ne (call $nextToken (local.get $tokenValuePtr)) (local.get $expectedToken))
      (then (throw $error (i32.const 15728720)))
    )
  )
  (func $compileExpr (result i32)
    (call $compileExpr6)
  )
  (export "compileExpr" (func $compileExpr))
  (func $compileStmt (result i32)
    (local $token i32)
    (local $tokenValuePtr i32)
    (local $outputPtr i32)
    (local $origInputPtr i32)

    (local.set $origInputPtr (global.get $inputPtr))
    (local.set $tokenValuePtr (call $alloc (i32.const 128)))
    (local.set $token (call $nextToken (local.get $tokenValuePtr)))

    (if (i32.eq (local.get $token) (i32.const 5)) ;; let
      (then
        (local.set $outputPtr (call $alloc (i32.const 1024)))
        (block $loop_end
          (loop $loop
            (call $expectToken (i32.const 3) (local.get $tokenValuePtr)) ;; identifier
            (call $addLocalVar (local.get $tokenValuePtr))
            (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15730468))
            (call $strcat (local.get $outputPtr) (i32.const 1024) (local.get $tokenValuePtr))
            (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15730500))
            (local.set $token (call $nextToken (local.get $tokenValuePtr)))
            (br_if $loop_end (i32.eq (local.get $token) (i32.const 59))) ;; ;
            (if (i32.eq (local.get $token) (i32.const 44)) ;; ,
              (then
                (call $strcat (local.get $outputPtr) (i32.const 1024) (i32.const 15729794))
                (br $loop)
              )
            )
            (throw $error (i32.const 15730436))
          )
        )
        (return (local.get $outputPtr))
      )
    )

    (global.set $inputPtr (local.get $origInputPtr))
    (local.set $outputPtr (call $compileExpr))
    (call $expectToken (i32.const 59) (local.get $tokenValuePtr)) ;; ;
    (local.get $outputPtr)
  )
  (export "compileStmt" (func $compileStmt))
  (func $compileBlock (param $indentLevel i32) (result i32)
    (local $token i32)
    (local $tokenValuePtr i32)
    (local $outputPtr i32)
    (local $origInputPtr i32)

    (local.set $origInputPtr (global.get $inputPtr))
    (local.set $tokenValuePtr (call $alloc (i32.const 128)))
    (local.set $token (call $nextToken (local.get $tokenValuePtr)))

    (local.set $outputPtr (call $alloc (i32.const 4096)))
    (if (i32.eq (local.get $token) (i32.const 123)) ;; {
      (then
        (block $loop_end
          (loop $loop
            (local.set $origInputPtr (global.get $inputPtr))
            (local.set $token (call $nextToken (local.get $tokenValuePtr)))
            (br_if $loop_end (i32.eq (local.get $token) (i32.const 125))) ;; }
            (global.set $inputPtr (local.get $origInputPtr))
            (call $strcat (local.get $outputPtr) (i32.const 4096) (call $indent (local.get $indentLevel)))
            (call $strcat (local.get $outputPtr) (i32.const 4096) (call $compileStmt))
            (call $strcat (local.get $outputPtr) (i32.const 4096) (i32.const 15730532))
            (br $loop)
          )
        )
      )
      (else
        (global.set $inputPtr (local.get $origInputPtr))
        (call $strcat (local.get $outputPtr) (i32.const 4096) (call $indent (local.get $indentLevel)))
        (call $strcat (local.get $outputPtr) (i32.const 4096) (call $compileStmt))
        (call $strcat (local.get $outputPtr) (i32.const 4096) (i32.const 15730532))
      )
    )
    (local.get $outputPtr)
  )
  (export "compileBlock" (func $compileBlock))
  (func $indent (param $indentLevel i32) (result i32)
    (local $outputPtr i32)
    (local $p i32)
    (local $i i32)
    (local.set $outputPtr
      (call $alloc (i32.add (i32.mul (local.get $indentLevel) (i32.const 2)) (i32.const 1)))
    )
    (local.set $p (local.get $outputPtr))
    (local.set $i (i32.const 0))
    (block $loop_end
      (loop $loop
        (br_if $loop_end (i32.ge_s (local.get $i) (local.get $indentLevel)))
        (i32.store8 (local.get $p) (i32.const 32))
        (i32.store8 (i32.add (local.get $p) (i32.const 1)) (i32.const 32))
        (local.set $i (i32.add (local.get $i) (i32.const 1)))
        (local.set $p (i32.add (local.get $p) (i32.const 2)))
        (br $loop)
      )
    )
    (i32.store8 (local.get $p) (i32.const 0))
    (local.get $outputPtr)
  )

  ;; Lexer.
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
    (local $p i32)
    (call $skipWhitespaceAndComments)
    (local.set $c (i32.load8_u (global.get $inputPtr)))

    ;; EOF
    (if (i32.eqz (local.get $c))
      (then (return (i32.const 0)))
    )

    ;; Number literal
    (if
      (i32.or
        (call $isdigit (local.get $c))
        (i32.and
          (i32.eq (local.get $c) (i32.const 45)) ;; -
          (call $isdigit (i32.load8_u (i32.add (global.get $inputPtr) (i32.const 1))))
        )
      )
      (then
        (local.set $p (local.get $tokenValuePtr))
        (loop $loop
          (if (i32.ge_u (i32.sub (local.get $p) (local.get $tokenValuePtr)) (i32.const 127))
            (then (throw $error (i32.const 15728864)))
          )
          (i32.store8 (local.get $p) (local.get $c))
          (local.set $p (i32.add (local.get $p) (i32.const 1)))
          (call $inputPtrInc)
          (local.set $c (i32.load8_u (global.get $inputPtr)))
          (br_if $loop (call $isdigit (local.get $c)))
        )
        (i32.store8 (local.get $p) (i32.const 0))
        (return (i32.const 1))
      )
    )

    ;; String literal
    (if (i32.eq (local.get $c) (i32.const 34)) ;; "
      (then
        (local.set $p (local.get $tokenValuePtr))
        (block $loop_end
          (loop $loop
            (if (i32.ge_u (i32.sub (local.get $p) (local.get $tokenValuePtr)) (i32.const 127))
              (then (throw $error (i32.const 15728864)))
            )
            (call $inputPtrInc)
            (local.set $c (i32.load8_u (global.get $inputPtr)))
            (if (i32.eqz (local.get $c))
              (then (throw $error (i32.const 15728656)))
            )
            (br_if $loop_end (i32.eq (local.get $c) (i32.const 34))) ;; "
            (i32.store8 (local.get $p) (local.get $c))
            (local.set $p (i32.add (local.get $p) (i32.const 1)))
            (br $loop)
          )
        )
        (i32.store8 (local.get $p) (i32.const 0))
        (call $inputPtrInc)
        (return (i32.const 2))
      )
    )

    ;; Identifier or keyword.
    (if (call $isident (local.get $c))
      (then
        (local.set $p (local.get $tokenValuePtr))
        (loop $loop
          (if (i32.ge_u (i32.sub (local.get $p) (local.get $tokenValuePtr)) (i32.const 127))
            (then (throw $error (i32.const 15728864)))
          )
          (i32.store8 (local.get $p) (local.get $c))
          (local.set $p (i32.add (local.get $p) (i32.const 1)))
          (call $inputPtrInc)
          (local.set $c (i32.load8_u (global.get $inputPtr)))
          (br_if $loop (call $isident (local.get $c)))
        )
        (i32.store8 (local.get $p) (i32.const 0))
        (return
          (i32.add
            (i32.const 4)
            (call $strlistFind (i32.const 15729248) (local.get $tokenValuePtr))
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
            (call $strlistFind (i32.const 15729504) (local.get $tokenValuePtr))
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
    (call $strcat (i32.const 15728768) (i32.const 32) (local.get $tokenValuePtr))
    (throw $error (i32.const 15728768))
  )
  (export "nextToken" (func $nextToken))
  (func $peekNextToken (param $tokenValuePtr i32) (result i32)
    (local $origInputPtr i32)
    (local $token i32)
    (local.set $origInputPtr (global.get $inputPtr))
    (local.set $token (call $nextToken (local.get $tokenValuePtr)))
    (global.set $inputPtr (local.get $origInputPtr))
    (return (local.get $token))
  )
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
  (func $strlistFind (param $strlist i32) (param $str i32) (result i32)
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
  (func $strlistAppend (param $strlist i32) (param $strlistSize i32) (param $str i32)
    (local $p i32)
    (local $len i32)
    (local.set $p (local.get $strlist))
    (block $loop_end
      (loop $loop
        (br_if $loop_end (i32.eqz (i32.load8_u (local.get $p))))
        (local.set $p
          (i32.add (i32.add (local.get $p) (call $strlen (local.get $p))) (i32.const 1))
        )
        (br $loop)
      )
    )
    (local.set $len (call $strlen (local.get $str)))
    (if
      (i32.ge_u
        (i32.add (i32.add (i32.sub (local.get $p) (local.get $strlist)) (local.get $len)) (i32.const 2))
        (local.get $strlistSize)
      )
      (then (throw $error (i32.const 15728896)))
    )
    (memory.copy
      (local.get $p)
      (local.get $str)
      (i32.add (local.get $len) (i32.const 1))
    )
    (i32.store8
      (i32.add (i32.add (local.get $p) (local.get $len)) (i32.const 2))
      (i32.const 0)
    )
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
  (func $strcat (param $str1 i32) (param $str1Size i32) (param $str2 i32)
    (local $len1 i32)
    (local $len2 i32)
    (local.set $len1 (call $strlen (local.get $str1)))
    (local.set $len2 (call $strlen (local.get $str2)))
    (if
      (i32.ge_u
        (i32.add (i32.add (local.get $len1) (local.get $len2)) (i32.const 1))
        (local.get $str1Size))
      (then (throw $error (i32.const 15728832)))
    )
    (memory.copy
      (i32.add (local.get $str1) (local.get $len1))
      (local.get $str2)
      (i32.add (local.get $len2) (i32.const 1))
    )
  )
  (func $strcpy (param $dest i32) (param $destSize i32) (param $src i32)
    (local $srcLen i32)
    (local.set $srcLen (call $strlen (local.get $src)))
    (if (i32.ge_u (i32.add (local.get $srcLen) (i32.const 1)) (local.get $destSize))
      (then (throw $error (i32.const 15728800)))
    )
    (memory.copy
      (local.get $dest)
      (local.get $src)
      (i32.add (call $strlen (local.get $src)) (i32.const 1))
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
    ;; Set up symbol table state
    (global.set $globalVars (call $alloc (i32.const 8192)))
    (global.set $localVars (call $alloc (i32.const 8192)))
    (global.set $fns (call $alloc (i32.const 8192)))
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
  (data (i32.const 15728688)
    "Expected expression\00"
  )
  (data (i32.const 15728720)
    "Unexpected token type\00"
  )
  (data (i32.const 15728768)
    "Unrecognized character: \00"
  )
  (data (i32.const 15728800)
    "strcpy out of bounds\00"
  )
  (data (i32.const 15728832)
    "strcat out of bounds\00"
  )
  (data (i32.const 15728864)
    "token length out of bounds\00"
  )
  (data (i32.const 15728896)
    "strlistAppend out of bounds\00"
  )
  ;; Keywords
  (data (i32.const 15729248)
    "fn\00"
    "let\00"
    "if\00"
    "else\00"
    "while\00"
    "return\00"
    "\00"
  )
  ;; 2-letter operators
  (data (i32.const 15729504)
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
  ;; WAT strings.
  (data (i32.const 15729792)
    ")\00"
  )
  (data (i32.const 15729794)
    " \00"
  )
  (data (i32.const 15729796)
    "(i32.const \00"
  )
  (data (i32.const 15729828)
    "(i32.mul \00"
  )
  (data (i32.const 15729860)
    "(i32.div_s \00"
  )
  (data (i32.const 15729892)
    "(i32.add \00"
  )
  (data (i32.const 15729924)
    "(i32.sub \00"
  )
  (data (i32.const 15729956)
    "(i32.eq \00"
  )
  (data (i32.const 15729988)
    "(i32.ne \00"
  )
  (data (i32.const 15730020)
    "(i32.gt_s \00"
  )
  (data (i32.const 15730052)
    "(i32.lt_s \00"
  )
  (data (i32.const 15730084)
    "(i32.ge_s \00"
  )
  (data (i32.const 15730116)
    "(i32.le_s \00"
  )
  (data (i32.const 15730148)
    "(i32.and \00"
  )
  (data (i32.const 15730180)
    "(i32.or \00"
  )
  (data (i32.const 15730212)
    "(i32.neg \00"
  )
  (data (i32.const 15730244)
    "(i32.xor (i32.const -1) \00"
  )
  (data (i32.const 15730276)
    "(local.get $\00"
  )
  (data (i32.const 15730308)
    "(global.get $\00"
  )
  (data (i32.const 15730340)
    "Unrecognized variable\00"
  )
  (data (i32.const 15730372)
    "(call $\00"
  )
  (data (i32.const 15730404)
    "Expected ',' or ')'\00"
  )
  (data (i32.const 15730436)
    "Expected ',' or ';'\00"
  )
  (data (i32.const 15730468)
    "(local $\00"
  )
  (data (i32.const 15730500)
    " i32)\00"
  )
  (data (i32.const 15730532)
    "\n\00"
  )
)