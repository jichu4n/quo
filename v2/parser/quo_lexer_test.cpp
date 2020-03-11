#include <cstdio>
#include <iostream>
#include <cassert>
#include <vector>
// Evil hack to get to the private token symbol table inside the parser...
#define private public
#include "parser/quo_parser.hpp"
#include "parser/quo_parser.cpp"
#include "parser/quo_lexer.cpp"
#undef private

using namespace std;
using namespace yy;

vector<string> RunLexAndGetTokenNames(const string& input) {
  yy_scan_string(input.c_str());
  vector<string> tokens;
  for (;;) {
    parser::symbol_type symbol(yylex());
    if (!symbol.type) {
      break;
    }
    tokens.push_back(parser::yytname_[symbol.type]);
  }
  return tokens;
}

void RunLexTest(const string& input, const vector<string>& expected_token_names) {
  const auto actual_token_names = RunLexAndGetTokenNames(input);
  cout << "Input: " << endl << input << endl;
  cout << "Expected: " << endl;
  for (const string &s : expected_token_names) {
    cout << s << " ";
  }
  cout << endl;
  cout << "Actual: " << endl;
  for (const string &s : actual_token_names) {
    cout << s << " ";
  }
  cout << endl;
  if (actual_token_names == expected_token_names) {
    cout << "Pass!" << endl;
  } else {
    cout << "Fail!" << endl;
    throw 1;
  }
}

int main() {
  RunLexTest(
      R"(
      // Sanity check
      fn f(): Int {
	let x: Int = 42;
	return x;
      }
      )", {
      "FUNCTION",
      "IDENTIFIER",
      "L_PAREN",
      "R_PAREN",
      "COLON",
      "IDENTIFIER",
      "L_BRACE",
      "LET",
      "IDENTIFIER",
      "COLON",
      "IDENTIFIER",
      "ASSIGN",
      "INTEGER_LITERAL",
      "SEMICOLON",
      "RETURN",
      "IDENTIFIER",
      "SEMICOLON",
      "R_BRACE",
  });
  RunLexTest(
      R"(
      "hello \"world!\""
      )", {
      "STRING_LITERAL",
  });
  return 0;
}

