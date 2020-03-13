find_package(BISON)
find_package(FLEX)

# =============================================================================
# quo_ast_types library
# =============================================================================
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
  inja
)

add_custom_command(
  OUTPUT
  quo_ast_types.cpp
  quo_ast_types.hpp
  COMMAND
  generate_quo_ast_types
  ARGS
  --ast_json_file_path=${CMAKE_CURRENT_SOURCE_DIR}/quo_ast.json
  --cpp_template_file_path=${CMAKE_CURRENT_SOURCE_DIR}/quo_ast_types_cpp_template.txt
  --hpp_template_file_path=${CMAKE_CURRENT_SOURCE_DIR}/quo_ast_types_hpp_template.txt
  --output_cpp_file_path=${CMAKE_CURRENT_BINARY_DIR}/quo_ast_types.cpp
  --output_hpp_file_path=${CMAKE_CURRENT_BINARY_DIR}/quo_ast_types.hpp
  DEPENDS
  generate_quo_ast_types
  quo_ast.json
  quo_ast_types_cpp_template.txt
  quo_ast_types_hpp_template.txt
)

add_library(
  quo_ast_types
  quo_ast_types.cpp
)
target_link_libraries(
  quo_ast_types
  quo_runtime
)

# =============================================================================
# quo_parser library
# =============================================================================
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
  ${BISON_QuoParser_OUTPUTS}
  ${FLEX_QuoLexer_OUTPUTS}
)
target_link_libraries(
  quo_parser
  quo_ast_types
)

# =============================================================================
# quo_lexer_test
# =============================================================================
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
  quo_ast_types
  absl::strings
  gtest_main
)
add_dependencies(
  quo_lexer_test
  quo_parser
)

