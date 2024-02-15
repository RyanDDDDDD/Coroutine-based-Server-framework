#ifndef __SERVER_MACRO_HPP__
#define __SERVER_MACRO_HPP__

#include <string.h>
#include <assert.h>
#include "util.hpp"


#define SERVER_ASSERT(x) \
    if (!(x)) { \
        SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "\n" << "ASSERTION FAILED: " #x \
            << "\nbacktrace:\n" \
            << Server::BacktraceToString(100, 2, "\t\t"); \
        assert(x); \
    }

#define SERVER_ASSERT_INFO(x, w) \
    if (!(x)) { \
        SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "\n" << "ASSERTION FAILED: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << Server::BacktraceToString(100, 2, "\t\t"); \
        assert(x); \
    }

#endif