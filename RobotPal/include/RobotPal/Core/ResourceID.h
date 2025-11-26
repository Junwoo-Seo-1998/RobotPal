// StringUtils.h
#pragma once
#include <string>

using ResourceID = unsigned int; // 32비트 정수

constexpr ResourceID HashString(const char* str) {
    // FNV-1a Hash Algorithm (Compile-time if possible)
    ResourceID hash = 2166136261u;
    while (*str) {
        hash ^= (unsigned char)(*str++);
        hash *= 16777619u;
    }
    return hash;
}

// std::string 오버로딩
inline ResourceID HashString(const std::string& str) {
    return HashString(str.c_str());
}