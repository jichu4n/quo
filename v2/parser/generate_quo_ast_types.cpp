#include <fstream>
#include <iostream>
#include <memory>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "nlohmann/json.hpp"

using namespace std;
using namespace absl;
using json = nlohmann::json;

ABSL_FLAG(string, ast_json_file_path, "", "Path to input AST JSON file");
ABSL_FLAG(string, output_cpp_file_path, "", "Path to output C++ source file");
ABSL_FLAG(string, output_hpp_file_path, "", "Path to output C++ header file");

int main(int argc, char** argv) {
  ParseCommandLine(argc, argv);

  // Parse input AST JSON file.
  unique_ptr<ifstream> ast_json_file(
      GetFlag(FLAGS_ast_json_file_path).empty()
          ? nullptr
          : new ifstream(GetFlag(FLAGS_ast_json_file_path)));
  istream& ast_json_stream = ast_json_file ? *ast_json_file : cin;
  json ast_json;
  ast_json_stream >> ast_json;

  // Open output file paths.
  unique_ptr<ofstream> output_cpp_file(
      GetFlag(FLAGS_output_cpp_file_path).empty()
          ? nullptr
          : new ofstream(GetFlag(FLAGS_output_cpp_file_path)));
  ostream& output_cpp_stream = output_cpp_file ? *output_cpp_file : cout;
  unique_ptr<ofstream> output_hpp_file(
      GetFlag(FLAGS_output_hpp_file_path).empty()
          ? nullptr
          : new ofstream(GetFlag(FLAGS_output_hpp_file_path)));
  ostream& output_hpp_stream = output_hpp_file ? *output_hpp_file : cout;

  return 0;
}

