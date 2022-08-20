#ifndef __QUO_RT_H__
#define __QUO_RT_H__

#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

typedef int64_t Int64;

class String : public ::std::string {
  public:
    String();
    String(const ::std::string& other);
    String(const String& other);
    String* replace(String* regex, String* replacement) const;
    String* toUpperCase();
};

template<typename T>
void Print(T* v) {
	::std::cout << *v;
}

template<typename T>
void Println(T* v) {
	::std::cout << *v << ::std::endl;
}

template<typename T>
class Array {
  public:
    void add(T element) {
      elements.push_back(element);
    }

    void addAll(Array<T>* otherElements) {
      elements.insert(
          elements.end(),
          otherElements->elements.begin(),
          otherElements->elements.end());
    }

    T& operator[](Int64 index) {
      return elements[index];
    }

    Int64* length() {
      return new Int64(elements.size());
    }

  private:
    ::std::vector<T> elements;
};

extern "C" {

extern String* ReadFile(String* path);
extern Int64* WriteFile(String* path, String* content);

extern String* Join(Array<String*>* strings);

}

#endif

