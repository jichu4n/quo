find_package(BISON)
find_package(FLEX)

BISON_TARGET(
  QuoParser
  quo_parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/quo_parser.cpp)
FLEX_TARGET(
  QuoLexer
  quo_lexer.l
  ${CMAKE_CURRENT_BINARY_DIR}/quo_lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(QuoLexer QuoParser)

add_library(
  parser
  ${BISON_QuoParser_OUTPUTS}
  ${FLEX_QuoLexer_OUTPUTS}
)