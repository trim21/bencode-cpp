#pragma once

#include <Python.h>

#define HPy_ssize_t Py_ssize_t
#define HPy PyObject *

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#ifdef BENCODE_DEBUG

#ifdef _MSC_VER
#define debug_print(fmt, ...)                                                                      \
                                                                                                   \
  do {                                                                                             \
    printf(__FILE__);                                                                              \
    printf(":");                                                                                   \
    printf("%d", __LINE__);                                                                        \
    printf("\t%s", __FUNCTION__);                                                                  \
    printf("\tDEBUG: ");                                                                           \
    printf(fmt, __VA_ARGS__);                                                                      \
    printf("\n");                                                                                  \
  } while (0)

#else

#define debug_print(fmt, ...)                                                                      \
  do {                                                                                             \
    printf(__FILE__);                                                                              \
    printf(":");                                                                                   \
    printf("%d", __LINE__);                                                                        \
    printf("\t%s\tDEBUG: ", __PRETTY_FUNCTION__);                                                  \
    printf(fmt, ##__VA_ARGS__);                                                                    \
    printf("\n");                                                                                  \
  } while (0)

#endif
#else

#define debug_print(fmt, ...)

#endif

struct EncodeError : public std::exception {
public:
  EncodeError(std::string msg) { s = msg; }

  const char *what() const throw() { return s.c_str(); }

private:
  std::string s;
};

struct DecodeError : public std::exception {
public:
  DecodeError(std::string msg) { s = msg; }

  const char *what() const throw() { return s.c_str(); }

private:
  std::string s;
};
