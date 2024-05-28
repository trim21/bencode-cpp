#include <Python.h>
#include <algorithm> // std::sort
#include <pybind11/pybind11.h>

#include "common.h"
#include "ctx.h"

namespace py = pybind11;

#define returnIfError(o)                                                                           \
    if (o)                                                                                         \
    return

static void encodeAny(Context *ctx, py::handle obj);

bool cmp(std::pair<std::string, py::handle> &a, std::pair<std::string, py::handle> &b) {
    return a.first < b.first;
}

static void encodeDict(Context *ctx, py::handle obj) {
    debug_print("encodeDict");
    ctx->writeChar('d');
    auto l = PyDict_Size(obj.ptr());
    if (l == 0) {
        ctx->writeChar('e');
        return;
    }

    std::vector<std::pair<std::string, py::handle>> m(l);
    auto items = PyDict_Items(obj.ptr());

    // smart pointer to dec_ref when function return
    auto ref = AutoFree(items);

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

    for (auto pair : m) {
        ctx->writeSize_t(pair.first.length());
        ctx->writeChar(':');
        ctx->write(pair.first.data(), pair.first.length());

        encodeAny(ctx, pair.second);
    }

    ctx->writeChar('e');
    return;
}

// slow path for types.MappingProxyType
static void encodeDictLike(Context *ctx, py::handle h) {
    debug_print("encodeDictLike");
    ctx->writeChar('d');
    debug_print("get object size");
    auto l = PyObject_Size(h.ptr());
    if (l == 0) {
        ctx->writeChar('e');
        return;
    }

    auto obj = h.cast<py::object>();

    std::vector<std::pair<std::string, py::handle>> m(l);
    debug_print("get items");
    auto items = obj.attr("items")();

    size_t index = 0;
    for (auto keyValue : items) {
        std::string repr = py::repr(keyValue);
        debug_print("%s", repr.c_str());
        auto key = PyTuple_GetItem(keyValue.ptr(), 0);
        auto value = PyTuple_GetItem(keyValue.ptr(), 1);

        if (!(PyUnicode_Check(key) || PyBytes_Check(key))) {
            throw EncodeError("dict keys must by keys");
        }

        debug_print("set items");
        m.at(index) = std::make_pair(py::handle(key).cast<std::string>(), py::handle(value));
        index++;
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

    for (auto pair : m) {
        ctx->writeSize_t(pair.first.length());
        ctx->writeChar(':');
        ctx->write(pair.first.data(), pair.first.length());

        encodeAny(ctx, pair.second);
    }

    ctx->writeChar('e');
    return;
}

static void encodeInt_fast(Context *ctx, long long val) {
    ctx->writeChar('i');
    ctx->writeLongLong(val);
    ctx->writeChar('e');
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

    ctx->writeChar('e');
}

//
static void encodeList(Context *ctx, const py::handle obj) {
    ctx->writeChar('l');

    HPy_ssize_t len = PyList_Size(obj.ptr());
    for (HPy_ssize_t i = 0; i < len; i++) {
        HPy o = PyList_GetItem(obj.ptr(), i);
        encodeAny(ctx, o);
    }

    ctx->writeChar('e');
}

static void encodeTuple(Context *ctx, py::handle obj) {
    ctx->writeChar('l');

    HPy_ssize_t len = PyTuple_Size(obj.ptr());
    for (HPy_ssize_t i = 0; i < len; i++) {
        HPy o = PyTuple_GetItem(obj.ptr(), i);
        encodeAny(ctx, o);
    }

    ctx->writeChar('e');
}

#define encodeComposeObject(ctx, obj, encoder)                                                     \
    do {                                                                                           \
        uintptr_t key = (uintptr_t)obj.ptr();                                                      \
        debug_print("put object %p to seen", key);                                                 \
        debug_print("after put object %p to seen", key);                                           \
        if (ctx->seen.find(key) != ctx->seen.end()) {                                              \
            debug_print("circular reference found");                                               \
            throw py::value_error("circular reference found");                                     \
        }                                                                                          \
        ctx->seen.insert(key);                                                                     \
        encoder(ctx, obj);                                                                         \
        ctx->seen.erase(key);                                                                      \
        return;                                                                                    \
    } while (0)

static void encodeAny(Context *ctx, const py::handle obj) {
    debug_print("encodeAny");

    if (obj.ptr() == Py_True) {
        debug_print("encode true");
        ctx->write("i1e", 3);
        return;
    }

    if (obj.ptr() == Py_False) {
        debug_print("encode false");
        ctx->write("i0e", 3);
        return;
    }

    if (PyBytes_Check(obj.ptr())) {
        debug_print("encode bytes");
        HPy_ssize_t size = 0;
        char *s;

        if (PyBytes_AsStringAndSize(obj.ptr(), &s, &size)) {
            throw std::runtime_error("failed to get contents of bytes");
        }

        debug_print("write buffer");
        ctx->writeSize_t(size);
        debug_print("write char");
        ctx->writeChar(':');
        debug_print("write content");
        ctx->write((const char *)s, size);
        return;
    }

    if (py::isinstance<py::str>(obj)) {
        debug_print("encode str");
        HPy_ssize_t size = 0;

        const char *s = PyUnicode_AsUTF8AndSize(obj.ptr(), &size);

        debug_print("write length");
        ctx->writeSize_t(size);
        debug_print("write char");
        ctx->writeChar(':');
        debug_print("write content");
        ctx->write(s, size);
        return;
    }

    if (PyLong_Check(obj.ptr())) {
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

    // types.MappingProxyType
    debug_print("test if mapping proxy");
    if (obj.ptr()->ob_type == &PyDictProxy_Type) {
        debug_print("encode mapping proxy");
        encodeComposeObject(ctx, obj, encodeDictLike);
    }

    // Unsupported type, raise TypeError
    std::string repr = py::repr(obj.get_type());

    std::string msg = "unsupported object " + repr;

    throw py::type_error(msg);
}

py::bytes bencode(py::object v) {
    debug_print("1");
    auto ctx = Context();

    encodeAny(&ctx, v);

    auto res = py::bytes(ctx.buf, ctx.index);

    return res;
}
