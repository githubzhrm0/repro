// Purpose: Test what binding different scalar types (template arguments) might
// look like with `pybind11`.
// Specifically, having a base class of <T, U>, and seeing if pybind11 can bind
// it "easily".

#include <cstddef>
#include <cmath>
#include <sstream>
#include <string>

#include <pybind11/cast.h>
#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "cpp/name_trait.h"

namespace py = pybind11;
using namespace py::literals;
using namespace std;

namespace scalar_type {

template <typename T = float, typename U = int16_t>
class Base;

}

using scalar_type::Base;
NAME_TRAIT_TPL(Base)

namespace scalar_type {

// Simple base class.
template <typename T, typename U>
class Base {
 public:
  Base(T t, U u)
    : t_(t),
      u_(u) {}
  template <typename Tc, typename Uc>
  Base(const Base<Tc, Uc>& other)
    : Base(static_cast<T>(other.t_),
           static_cast<U>(other.u_)) {}

  T t() const { return t_; }
  U u() const { return u_; }

  virtual U pure(T value) const = 0;
  virtual U optional(T value) const {
    cout << py_name() << endl;
    return static_cast<U>(value);
  }

  U dispatch(T value) const {
    cout << "cpp.dispatch [" << py_name() << "]:\n";
    cout << "  ";
    U pv = pure(value);
    cout << "  ";
    U ov = optional(value);
    return pv + ov;
  }

  // TODO: Use `typeid()` and dynamic dispatching?
  static string py_name() {
    return "Base__T_" + name_trait<T>::name() +
      "__U_" + name_trait<U>::name();
  }

 private:
  template <typename Tc, typename Uc> friend class Base;
  T t_{};
  U u_{};
};

template <typename T, typename U>
class PyBase : public Base<T, U> {
 public:
  typedef Base<T, U> B;

  using B::B;

  U pure(T value) const override {
    PYBIND11_OVERLOAD_PURE(U, B, pure, value);
  }
  U optional(T value) const override {
    PYBIND11_OVERLOAD(U, B, optional, value);
  }
};

template <typename T, typename U>
void call_method(const Base<T, U>& base) {
  base.dispatch(T{});
}

/// Retuns the PyTypeObject from the resultant expression type.
/// @note This may incur a copy due to implementation details.
template <typename T>
py::handle py_type_eval(const T& value) {
  auto locals = py::dict("model_value"_a=value);
  return py::eval("type(model_value)", py::object(), locals);
}

template <typename T, bool is_default_constructible = false>
struct py_type_impl {
  static py::handle run() {
    // Check registered C++ types.
    const auto& internals = py::detail::get_internals();
    const auto& types_cpp = internals.registered_types_cpp;
    auto iter = types_cpp.find(std::type_index(typeid(T)));
    if (iter != types_cpp.end()) {
      auto info = static_cast<const py::detail::type_info*>(iter->second);
      return py::handle((PyObject*)info->type);
    } else {
      return py::handle();
    }
  }
};

template <typename T>
struct py_type_impl<T, true> {
  static py::handle run() {
    // First check registration.
    py::handle attempt = py_type_impl<T, false>::run();
    if (attempt) {
      return attempt;
    } else {
      // Next, check through default construction and using cast
      // implementations.
      try {
        return py_type_eval(T{});
      } catch (const py::cast_error&) {
        return py::handle();
      }
    }
  }
};

/// Gets the PyTypeObject representing the Python-compatible type for `T`.
/// @note If this is a custom type, ensure that it has been fully registered
/// before calling this.
/// @note If this is a builtin type, note that some information may be lost.
///   e.g. T=vector<int>  ->  <type 'list'>
///        T=int64_t      ->  <type 'long'>
///        T=int16_t      ->  <type 'long'>
///        T=double       ->  <type 'float'>
///        T=float        ->  <type 'float'>
template <typename T>
py::handle py_type() {
  py::handle type =
        py_type_impl<T, std::is_default_constructible<T>::value>::run();
  if (type) {
    return type;
  } else {
    throw std::runtime_error("Could not find type of: " + py::type_id<T>());
  }
}

struct A {
  explicit A(int x) {}
};

template <typename T, typename U>
void register_base(py::module m) {
  string name = Base<T, U>::py_name();
  typedef Base<T, U> C;
  typedef PyBase<T, U> PyC;
  py::class_<C, PyC> base(m, name.c_str());
  base
    .def(py::init<T, U>())
    .def("t", &C::t)
    .def("u", &C::u)
    .def("pure", &C::pure)
    .def("optional", &C::optional)
    .def("dispatch", &C::dispatch);

  // // Can't figure this out...
  // Can't get `overload_cast` to infer `Return` type.
  typedef void (*call_method_t)(const Base<T, U>&);
  m.def("call_method", static_cast<call_method_t>(&call_method));

  // http://pybind11.readthedocs.io/en/stable/advanced/pycpp/object.html#casting-back-and-forth
  // http://pybind11.readthedocs.io/en/stable/advanced/pycpp/utilities.html
  // http://pybind11.readthedocs.io/en/stable/advanced/misc.html
  auto type_tup = py::dict(
      "T"_a=py_type<T>(), "U"_a=py_type<U>(),
      "Z"_a=py_type<vector<T>>(),
      "Y"_a=py_type<A>()
  );
  auto locals = py::dict("cls"_a=base, "type_tup"_a=type_tup);
  auto globals = m.attr("__dict__");
  py::eval<py::eval_statements>(R"(#
cls.type_tup = type_tup
)", globals, locals);

  // registered_types_py
  // registered_types_cpp
  // registered_instances

  // // Register the type in Python.
  // // TODO: How to execute Python with arguments?
  // // How to convert a C++ typeid to a Python type, and vice versa?
  // // py::detail::type_info, detail::get_type_info
  // auto cls = py::exec();
  // auto locals = py::dict("params"_a=types, "cls"_a=cls);
  // py::exec(R"(
  //   _base_types[
  // )", m);
}

PYBIND11_PLUGIN(_scalar_type) {
  py::module m("_scalar_type", "Simple check on scalar / template types");

  py::class_<A> a(m, "A");

  auto globals = m.attr("__dict__");
  py::eval<py::eval_statements>(R"(#
# Dictionary.
#   Key: (T, U)
#   Value: PyType
base_types = {}
)", globals);

  // No difference between (float, double) and (int16_t, int64_t)
  // Gonna use other combos.
  register_base<double, int>(m);
  register_base<int, double>(m);

  return m.ptr();
}

}  // namespace scalar_type