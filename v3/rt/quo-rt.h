#ifndef __QUO_RT_H__
#define __QUO_RT_H__

#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

typedef int64_t Int64;
typedef ::std::string String;

template<typename T>
void Print(T* v) {
	::std::cout << *v;
}

template<typename T>
class Array {
	private:
	  ::std::vector<T> elements;
};

#endif
