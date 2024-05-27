#include <stdio.h>

#include <Python.h>
#include <algorithm> // std::sort
#include <pybind11/pybind11.h>

#include "common.h"
#include "ctx.h"

namespace py = pybind11;

#define returnIfError(o)                                                                           \
  if (o)                                                                                           \
  return

void encodeAny(Context *ctx, py::handle obj);

bool cmp(std::pair<std::string, py::handle> &a, std::pair<std::string, py::handle> &b) {
    return a.first < b.first;
}

static void encodeDict(Context *ctx, py::handle obj) {
    debug_print("encodeDict");
    returnIfError(bufferWrite(ctx, "d", 1));
    auto l = PyDict_Size(obj.ptr());
    if (l == 0) {
        bufferWrite(ctx, "e", 1);
        return;
    }

    std::vector<std::pair<std::string, py::handle>> m(l);
    auto items = PyDict_Items(obj.ptr());

    // smart pointer for ref cnt.
    auto ref = py::handle(items).cast<py::object>();
    ref.dec_ref();

    for (int i = 0; i < l; ++i) {
        auto keyValue = PyList_GetItem(items, i);
        auto key = PyTuple_GetItem(keyValue, 0);
        auto value = PyTuple_GetItem(keyValue, 1);

        if (!(PyUnicode_Check(key) || PyBytes_Check(key))) {
            throw EncodeError("dict keys must by keys");
        }

        debug_print("set items");
        m.at(i) = std::make_pair(py::handle(key).cast<std::string>(), py::handle(value));
    }

    std::sort(m.begin(), m.end(), cmp);
    auto lastKey = m[0].first;
    debug_print("key '%s'\n", lastKey.data());
    for (auto i = 1; i < l; i++) {
        auto currentKey = m[i].first;
        debug_print("key '%s'\n", currentKey.data());
        if (currentKey == lastKey) {
            throw EncodeError("found duplicated keys");
        }

        lastKey = currentKey;
    }

    for (auto pair: m) {
        ctx->writeSize_t(pair.first.length());
        bufferWriteChar(ctx, ':');
        bufferWrite(ctx, pair.first.data(), pair.first.length());

        encodeAny(ctx, pair.second);
    }

    bufferWrite(ctx, "e", 1);
    return;
}

static void encodeInt_fast(Context *ctx, long long val) {
    bufferWrite(ctx, "i", 1);
    ctx->writeLongLong(val);
    bufferWrite(ctx, "e", 1);
}

static void encodeInt_slow(Context *ctx, py::handle obj);

static void encodeInt(Context *ctx, py::handle obj) {
    int overflow = 0;
    long long val = PyLong_AsLongLongAndOverflow(obj.ptr(), &overflow);
    if (overflow) {
        PyErr_Clear();
        // slow path for very long int
        return encodeInt_slow(ctx, obj);
    }
    if (val == -1 && PyErr_Occurred()) { // unexpected error
        return;
    }

    return encodeInt_fast(ctx, val);
}

static void encodeInt_slow(Context *ctx, py::handle obj) {
    HPy fmt = PyUnicode_FromString("%d");
    HPy s = PyUnicode_Format(fmt, obj.ptr()); // s = '%d" % i
    if (s == NULL) {
        Py_DecRef(fmt);
        return;
    }

    // TODO: use PyUnicode_AsUTF8String in py>=3.10
    HPy b = PyUnicode_AsUTF8String(s); // b = s.encode()
    if (b == NULL) {
        Py_DecRef(fmt);
        Py_DecRef(s);
        return;
    }

    HPy_ssize_t size = PyBytes_Size(b); // len(b)
    const char *data = PyBytes_AsString(b);
    if (data == NULL) {
        Py_DecRef(fmt);
        Py_DecRef(s);
        Py_DecRef(b);
        return;
    }

    ctx->writeChar('i');
    ctx->write(data, size);

    Py_DecRef(fmt);
    Py_DecRef(s);
    Py_DecRef(b);

    bufferWrite(ctx, "e", 1);
}

//
static void encodeList(Context *ctx, const py::handle obj) {
    bufferWrite(ctx, "l", 1);

    HPy_ssize_t len = PyList_Size(obj.ptr());
    for (HPy_ssize_t i = 0; i < len; i++) {
        HPy o = PyList_GetItem(obj.ptr(), i);
        encodeAny(ctx, o);
    }

    bufferWrite(ctx, "e", 1);
}

static void encodeTuple(Context *ctx, py::handle obj) {
    bufferWrite(ctx, "l", 1);

    HPy_ssize_t len = PyTuple_Size(obj.ptr());
    for (HPy_ssize_t i = 0; i < len; i++) {
        HPy o = PyTuple_GetItem(obj.ptr(), i);
        encodeAny(ctx, o);
    }

    bufferWrite(ctx, "e", 1);
}

#define encodeComposeObject(ctx, obj, encoder)                                                     \
  do {                                                                                             \
    uintptr_t key = (uintptr_t)obj.ptr();                                                          \
    debug_print("put object %p to seen", key);                                                     \
    debug_print("after put object %p to seen", key);                                               \
    if (ctx->seen.contains(key)) {                                                                \
      debug_print("circular reference found");                                                     \
      throw py::value_error("circular reference found");                                           \
    }                                                                                              \
    ctx->seen.insert(key);                                                                         \
    encoder(ctx, obj);                                                                             \
    ctx->seen.erase(key);                                                                          \
    return;                                                                                        \
  } while (0)

static void encodeAny(Context *ctx, const py::handle obj) {
    debug_print("encodeAny");

    if (obj.ptr() == Py_True) {
        debug_print("encode true");
        bufferWrite(ctx, "i1e", 3);
        return;
    }

    if (obj.ptr() == Py_False) {
        debug_print("encode false");
        bufferWrite(ctx, "i0e", 3);
        return;
    }

    if (PyBytes_Check(obj.ptr())) {
        debug_print("encode bytes");
        HPy_ssize_t size = 0;
        char *s;

        if (PyBytes_AsStringAndSize(obj.ptr(), &s, &size)) {
            throw std::exception("failed to get contents of bytes");
        }

        debug_print("write buffer");
        ctx->writeSize_t(size);
        debug_print("write char");
        bufferWriteChar(ctx, ':');
        debug_print("write content");
        bufferWrite(ctx, (const char *) s, size);
        return;
    }

    if (py::isinstance<py::str>(obj)) {
        debug_print("encode str");
        HPy_ssize_t size = 0;

        const char *s = PyUnicode_AsUTF8AndSize(obj.ptr(), &size);

        debug_print("write length");
        ctx->writeSize_t(size);
        debug_print("write char");
        bufferWriteChar(ctx, ':');
        debug_print("write content");
        bufferWrite(ctx, s, size);
        return;
    }

    if (py::isinstance<py::int_>(obj)) {
        return encodeInt(ctx, obj);
    }

    if (PyList_Check(obj.ptr())) {
        encodeComposeObject(ctx, obj, encodeList);
    }

    if (PyTuple_Check(obj.ptr())) {
        encodeComposeObject(ctx, obj, encodeTuple);
    }

    if (PyDict_Check(obj.ptr())) {
        encodeComposeObject(ctx, obj, encodeDict);
    }

    // Unsupported type, raise TypeError
    std::string repr = py::repr(obj.get_type());

    std::string msg = std::format("failed to encode object {}", repr);

    throw py::type_error(msg);
}

py::bytes bencode(py::object v) {
    debug_print("1");
    auto ctx = Context();

    encodeAny(&ctx, v);

    auto res = py::bytes(ctx.buf, ctx.index);

    return res;
}