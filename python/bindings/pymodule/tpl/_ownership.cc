// Purpose: Test what avenues might be possible for creating instances in Python
// to then be owned in C++.

#include <cstddef>
#include <cmath>
#include <sstream>
#include <string>

#include <pybind11/cast.h>
#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace py::literals;
using namespace std;

namespace ownership {

template <typename T>
class Base {
 public:
  Base(int value)
      : value_(value) {}

  int value() const { return value_; }

 private:
  int value_{};
};

class A_ {};
class B_ {};

typedef Base<A_> A;
typedef Base<B_> B;

unique_ptr<A> check_creation_a(py::function py_factory, bool do_copy) {
  // unique_ptr<A> in = py::cast<unique_ptr<A>>(py_factory());  // Does not work.
  // BOTH of these cause issues...
  A* in{};
  auto getrefcount = py::module::import("sys").attr("getrefcount");
  {
    py::object py_in = py_factory();
    cout << "ref count: " << getrefcount(py_in).cast<int>() << endl;
    cout << "ref count (tmp): " << getrefcount(py_factory()).cast<int>() << endl;
    in = py::cast<A*>(py_in);
  }
  if (do_copy) {
    // This should be fine-ish.
    unique_ptr<A> out(new A(in->value() * 2));
    return out;
  } else {
    // Should cause an error.
    return unique_ptr<A>(in);
    // return in;
  }
}

shared_ptr<B> check_creation_b(py::function py_factory, bool do_copy) {
  shared_ptr<B> in;
  auto getrefcount = py::module::import("sys").attr("getrefcount");
  {
    py::object py_in = py_factory();
    cout << "ref count: " << getrefcount(py_in).cast<int>() << endl;
    cout << "ref count (tmp): " << getrefcount(py_factory()).cast<int>() << endl;
    in = py::cast<shared_ptr<B>>(py_factory());
  }
  if (do_copy) {
    // This should be fine.
    shared_ptr<B> out(new B(in->value() * 2));
    return out;
  } else {
    // Should work as well?
    return in;
  }
}

PYBIND11_PLUGIN(_ownership) {
  py::module m("_ownership", "Check ownership possibilites");

  py::class_<A>(m, "A")
    .def(py::init<int>())
    .def("value", &A::value);
  py::class_<B, std::shared_ptr<B>>(m, "B")
    .def(py::init<int>())
    .def("value", &B::value);

  m.def("check_creation_a", &check_creation_a);
  m.def("check_creation_b", &check_creation_b);

  return m.ptr();
}

}  // namespace scalar_type

