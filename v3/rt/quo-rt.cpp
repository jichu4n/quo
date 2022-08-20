#include "quo-rt.h"

#include <fstream>
#include <sstream>
#include <regex>

using namespace std;

String::String(): string() {}

String::String(const string& other)
    : string(other) {
}

String::String(const String& other)
    : string(other) {
}

String* String::replace(String* regexString, String* replacementString) const {
  regex re(*regexString);
  return new String(regex_replace(*this, re, *replacementString));
}

String* String::toUpperCase() {
  ostringstream buffer;
  for (int i = 0; i < length(); ++i) {
    buffer << static_cast<char>(toupper((*this)[i]));
  }
  return new String(buffer.str());
}

String* ReadFile(String* path) {
  ifstream inputFile(*path);
  if (inputFile.fail()) {
    return nullptr;
  }
  stringstream buffer;
  buffer << inputFile.rdbuf();
  return new String(buffer.str());
}

Int64* WriteFile(String* path, String* content) {
  ofstream outputFile(*path);
  if (outputFile.fail()) {
    return new Int64(false);
  }
  outputFile << *content;
  outputFile.close();
  return new Int64(true);
}

String* Join(Array<String*>* strings) {
  ostringstream buffer;
  const Int64 length = *strings->length();
  for (Int64 i = 0; i < length; ++i) {
    buffer << *((*strings)[i]);
  }
  return new String(buffer.str());
}

extern "C" {

Int64* Main(Array<String*>* argv);

}

int main(int argc, char* argv[]) {
  auto converted_argv = new Array<String*>();
  for (int i = 0; i < argc; ++i) {
    converted_argv->add(new String(argv[i]));
  }
  auto return_code = Main(converted_argv);
  return return_code == nullptr ? 0 : *return_code;
}
