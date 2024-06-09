#include <iostream>

#include "emscripten/bind.h"
#include "emscripten/val.h"

class MyObject {
public:
  int (*f)(int);
  static int magicNumber(int k) {
    return 42*k;
  }
  MyObject() {
    f = magicNumber;
  }
  int Execute(int k) {
    return this->f(k);
  }  
};

int add (int a, int b, emscripten::val cb) {
  return cb.call<int>("call", 0, a + b);
};

// Binding code
EMSCRIPTEN_BINDINGS(functionptr_prototype) {
  emscripten::class_<MyObject>("MyObject")
    .constructor<>()
    .function("Execute", &MyObject::Execute);
  emscripten::function("add", &add);
}
