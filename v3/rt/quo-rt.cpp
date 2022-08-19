#include "quo-rt.h"

#include <fstream>
#include <sstream>

using namespace std;

String* ReadFile(String* path) {
  ifstream inputFile(*path);
  if (inputFile.fail()) {
    return nullptr;
  }
  stringstream buffer;
  buffer << inputFile.rdbuf();
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
