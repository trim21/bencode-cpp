#pragma once

#include <string>
#include <unordered_set>

#include <fmt/core.h>

#include "common.h"

#define defaultBufferSize 4096

class BufferAllocFailed : std::bad_alloc {
    const char *what() const throw() { return "failed to alloc member for buffer"; }
};

class Context {
public:
    char *buf;
    size_t index;
    size_t cap;
    std::unordered_set<uintptr_t> seen;

    Context() {
        buf = (char *) malloc(defaultBufferSize);
        if (buf == NULL) {
            throw BufferAllocFailed();
        }

        index = 0;
        cap = defaultBufferSize;
    }

    ~Context() {
        seen.clear();
        free(buf);
    }

    void write(const char *data, HPy_ssize_t size) {
        bufferGrow(size);

        std::memcpy(buf + index, data, size);

        index = index + size;
    }

    void writeSize_t(size_t val) {
        std::string s = fmt::format("{}", val);
        write(s.data(), s.length());
    }

    void writeLongLong(long long val) {
        std::string s = fmt::format("{}", val);
        write(s.data(), s.length());
    }

    void writeChar(const char c) {
        bufferGrow(1);
        buf[index] = c;
        index = index + 1;
    }

private:
    void bufferGrow(HPy_ssize_t size) {
        if (size + index + 1 >= cap) {
            char *tmp = (char *) realloc(buf, cap * 2 + size);
            if (tmp == NULL) {
                throw BufferAllocFailed();
            }
            cap = cap * 2 + size;
            buf = tmp;
        }
    }
};

static int bufferWrite(Context *ctx, const char *data, HPy_ssize_t size) {
    ctx->write(data, size);
    return 0;
}

static int bufferWriteChar(Context *ctx, const char c) {
    ctx->writeChar(c);
    return 0;
}
