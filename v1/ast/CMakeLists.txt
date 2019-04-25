# protobuf_generate_python(ast_PY_SRCS ast.proto)
# add_custom_target(ast_PY ALL DEPENDS ${ast_PY_SRCS})
# file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/__init__.py")

protobuf_generate_cpp(ast_CPP_SRCS ast_CPP_HDRS ast.proto)
add_library(ast ${ast_CPP_HDRS} ${ast_CPP_SRCS})
