// StringUtils.h
#ifndef __RESOURCEID_H__
#define __RESOURCEID_H__
#include <string>

using ResourceID = int;

constexpr ResourceID GetID(const char* str) {
    // FNV-1a Hash Algorithm (Compile-time if possible)
    ResourceID hash = 2166136261u;
    while (*str) {
        hash ^= (unsigned char)(*str++);
        hash *= 16777619u;
    }
    return hash;
}

// std::string 오버로딩
inline ResourceID GetID(const std::string& str) {
    return GetID(str.c_str());
}

#endif