
/* C++ module generated by Quo C++ translator.
 * Timestamp: {{ ts }}
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

typedef int Int;
typedef ::std::string String;


template<typename T>
Int Print(const T& t) {
  ::std::cout << t;
  return 0;
}

template<typename T>
class Array {
  public:
    ::std::unique_ptr<T>& operator [] (int idx) {
      return vector_[idx];
    }
    void Append(const T& t) {
      vector_.emplace_back(new T(t));
    }
    void Append(::std::unique_ptr<T> t) {
      vector_.emplace_back(std::move(t));
    }
    Int Length() {
      return vector_.size();
    }
  private:
    ::std::vector<::std::unique_ptr<T>> vector_;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

{{ content }}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

