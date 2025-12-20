#pragma once

#include <fmt/base.h>
#include <fmt/format.h>

template <typename... T>
void not_implemented(fmt::format_string<T...> fmt, T&&... args) {
    auto s = fmt::vformat(fmt, fmt::make_format_args(args...));
    printf("not implemented: %s\n", s);
    exit(1);
}

template <typename... T>
void panic(fmt::format_string<T...> fmt, T&&... args) {
    auto s = fmt::vformat(fmt, fmt::make_format_args(args...));
    printf("panic: %s\n", s);
    exit(1);
}

template <typename... T>
void log(fmt::format_string<T...> fmt, T&&... args) {
    auto s = fmt::vformat(fmt, fmt::make_format_args(args...));
    printf("%s\n", s.c_str());
}

template <typename... T>
void assert(bool stmt, fmt::format_string<T...> fmt, T&&... args) {
    if (!stmt) [[unlikely]] {
        auto s = fmt::vformat(fmt, fmt::make_format_args(args...));
        printf("assertion failed: %s\n", s.c_str());
        exit(1);
    }
}
