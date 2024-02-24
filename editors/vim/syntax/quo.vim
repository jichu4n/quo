syntax match quoComment "//.*$"
syntax region quoString start='"' end='"'
syntax match quoNumber "\v<\d+>"
syntax keyword quoKeywords
  \ fn
  \ let
  \ if
  \ else
  \ while
  \ return
  \ break
  \ continue
  \ class
  \ new
  \ delete

highlight default link quoComment Comment
highlight default link quoString Constant
highlight default link quoNumber Constant
highlight default link quoOperators Operator
highlight default link quoKeywords Keyword

