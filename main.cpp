#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkCallbackCommand.h"
#include <iostream>
#include <new>

#include "emscripten/bind.h"

class vtkCallbackCommand;

namespace emscripten {
// Teaches emscripten to work with vtkSmartPointer. All vtkObjects will
// be constructed through vtkSmartPointer<T>::New
template <typename T> struct smart_ptr_trait<vtkSmartPointer<T>> {
  using pointer_type = vtkSmartPointer<T>;
  using element_type = T;

  static sharing_policy get_sharing_policy() {
    // Intrusive because element_type is a vtkObject derived instance which
    // keeps reference count.
    return sharing_policy::INTRUSIVE;
  }

  static element_type *get(const pointer_type &p) { return p.Get(); }

  static pointer_type share(const pointer_type &r, T *ptr) {
    return pointer_type(ptr);
  }

  static pointer_type *construct_null() { return new pointer_type; }
};

#if 0
  namespace internal {
    template<> void raw_destructor<vtkCallbackCommand>(vtkCallbackCommand* ptr) { ptr->Delete(); }
  }
  #endif
} // namespace emscripten

// Example custom object derived vtkObject
class vtkCustomObject : public vtkObject {
public:
  vtkTypeMacro(vtkCustomObject, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent) override {}
  static vtkCustomObject *New();

protected:
  vtkCustomObject(){
    vtkLog(INFO, << "Constructed " << vtkLogIdentifier(this));
  }
  ~vtkCustomObject() {
    vtkLog(INFO, << "Destroyed " << vtkLogIdentifier(this));
  }

private:
  vtkCustomObject(const vtkCustomObject &) = delete;
  void operator=(const vtkCustomObject &) = delete;
};

vtkStandardNewMacro(vtkCustomObject);

// Teaches emscripten how to delete vtkObject derived instances.
#define vtkAddDestructor(cname)                                                \
  template <> void emscripten::internal::raw_destructor<cname>(cname * ptr) {  \
    ptr->Delete();                                                             \
  }

vtkAddDestructor(vtkObjectBase);
vtkAddDestructor(vtkObject);
vtkAddDestructor(vtkCustomObject);

// Similar to std::make_shared<T>
template <typename T> vtkSmartPointer<T> MakeVTKSmartPtr() {
  return vtkSmartPointer<T>::New();
}

#if 0
class jsCallbackCommand : public vtkCallbackCommand {
public:
  static jsCallbackCommand* New()
  {
    return new jsCallbackCommand;
  }

  virtual void Execute(vtkObject* caller, unsigned long evId, void*) = 0;

protected:
  jsCallbackCommand() = default;
private:
  jsCallbackCommand(const jsCallbackCommand&) = delete;
  void operator=(const jsCallbackCommand&) = delete;
};
#endif

struct Interface {
    virtual void invoke(const std::string& str) = 0;
};

struct InterfaceWrapper : public emscripten::wrapper<Interface> {
    EMSCRIPTEN_WRAPPER(InterfaceWrapper);
    void invoke(const std::string& str) {
        return call<void>("invoke", str);
    }
};

class vtkCallbackCommandWrapper : public emscripten::wrapper<vtkCallbackCommand> {
public:
  EMSCRIPTEN_WRAPPER(vtkCallbackCommandWrapper);
  void Execute(vtkObject* caller, unsigned long eid, void* callData) {
    return call<void>("Execute", caller, eid, callData);
  }
private:
  // vtkCallbackCommandWrapper()  {} /* How to explicitly initialize base class */
};
vtkStandardNewMacro(vtkCallbackCommandWrapper);


vtkAddDestructor(vtkCallbackCommand);
// Does this cause double deletion
vtkAddDestructor(emscripten::wrapper<vtkCallbackCommand>);
vtkAddDestructor(vtkCallbackCommandWrapper);


struct Base {
    virtual void invoke(const std::string& str) {
        // default implementation
    }
};

struct BaseWrapper : public emscripten::wrapper<Base> {
    EMSCRIPTEN_WRAPPER(BaseWrapper);
    void invoke(const std::string& str) {
        return call<void>("invoke", str);
    }
};

// Binding code
EMSCRIPTEN_BINDINGS(vtksmartptr_prototype) {
  // Non-abstract virtual methods
  emscripten::class_<Base>("Base")
    .allow_subclass<BaseWrapper>("BaseWrapper")
    .function("invoke", emscripten::optional_override([](Base& self, const std::string& str) {
      return self.Base::invoke(str);
    }));
  emscripten::class_<Interface>("Interface")
    .function("invoke", &Interface::invoke, emscripten::pure_virtual())
    .allow_subclass<InterfaceWrapper>("InterfaceWrapper");
  // TODO: Is the subclass correctly destroyed??
  emscripten::class_<vtkCallbackCommand>("vtkCallbackCommand")
    .constructor(&vtkCallbackCommand::New, emscripten::allow_raw_pointers())
    .allow_subclass<vtkCallbackCommandWrapper>("vtkCallbackCommandWrapper")
    .constructor(&vtkCallbackCommandWrapper::New, emscripten::allow_raw_pointers())
    .function("Execute", emscripten::optional_override([](vtkCallbackCommand& self, vtkObject* caller, unsigned long eid, void* callData) {
        return self.vtkCallbackCommand::Execute(caller, eid, callData);
    }), emscripten::allow_raw_pointers());
  emscripten::class_<vtkObjectBase>("vtkObjectBase");
  emscripten::class_<vtkObject, emscripten::base<vtkObjectBase>>("vtkObject")
      .smart_ptr<vtkSmartPointer<vtkObject>>("vtkSmartPointer<vtkObject>")
      .constructor(&MakeVTKSmartPtr<vtkObject>)
      .function("GetReferenceCount", &vtkObject::GetReferenceCount);
  emscripten::class_<vtkCustomObject, emscripten::base<vtkObject>>(
      "vtkCustomObject")
      .smart_ptr<vtkSmartPointer<vtkCustomObject>>("vtkSmartPointer<vtkCustomObject>")
      .constructor(&MakeVTKSmartPtr<vtkCustomObject>);
}
