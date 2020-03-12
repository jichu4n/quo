find_package(BISON)
find_package(FLEX)

add_executable(
  generate_quo_ast_types
  generate_quo_ast_types.cpp
)
target_link_libraries(
  generate_quo_ast_types
  quo_runtime
  absl::flags
  absl::flags_parse
  nlohmann_json::nlohmann_json
)

add_custom_command(
  OUTPUT
  quo_ast_types.cpp
  quo_ast_types.hpp
  COMMAND
  generate_quo_ast_types
  ARGS
  --ast_json_file_path=${CMAKE_CURRENT_SOURCE_DIR}/quo_ast.json
  --output_cpp_file_path=${CMAKE_CURRENT_BINARY_DIR}/quo_ast_types.cpp
  --output_hpp_file_path=${CMAKE_CURRENT_BINARY_DIR}/quo_ast_types.hpp
  DEPENDS
  generate_quo_ast_types
)

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
  quo_parser
  quo_ast_types.cpp
  ${BISON_QuoParser_OUTPUTS}
  ${FLEX_QuoLexer_OUTPUTS}
)

add_executable(
  quo_lexer_test
  quo_lexer_test.cpp
)
add_test(
  quo_lexer_test
  quo_lexer_test
)
target_link_libraries(
  quo_lexer_test
  absl::strings
  gtest_main
)
add_dependencies(
  quo_lexer_test
  quo_parser
)
