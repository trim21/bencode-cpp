#include <pybind11/pybind11.h>

#include "common.h"

namespace py = pybind11;

extern py::bytes bencode(py::object v);

extern py::object bdecode(py::object b);

PYBIND11_MODULE(_bencode, m) {
    m.def("bdecode", &bdecode, "");
    m.def("bencode", &bencode, "");
    py::register_exception<DecodeError>(m, "BencodeDecodeError");
    py::register_exception<EncodeError>(m, "BencodeEncodeError");
}
