// StringUtils.h
#ifndef __RESOURCEID_H__
#define __RESOURCEID_H__
#include <string>
#include <string_view>
#include <cstdint>
#include <functional> // std::hash

// 64-bit FNV-1a Constants
constexpr uint64_t FNV_OFFSET_BASIS_64 = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME_64 = 1099511628211ULL;

struct ResourceID {
    uint64_t value;

    // 1. 기본 생성자 -> Invalid(0)
    constexpr ResourceID() : value(0) {}

    // 2. 값 직접 주입 (복구용)
    constexpr explicit ResourceID(uint64_t v) : value(v) {}

    // 3. 문자열 생성자 (해싱)
    constexpr ResourceID(std::string_view str) : value(0) {
        if (!str.empty()) {
            uint64_t hash = FNV_OFFSET_BASIS_64;
            for (char c : str) {
                hash ^= static_cast<unsigned char>(c);
                hash *= FNV_PRIME_64;
            }
            value = (hash == 0) ? 1 : hash;
        }
    }

    // 4. [핵심] if (id) 지원
    // explicit 키워드가 있어야 의도치 않은 정수 연산(id + 1 등)을 막아줌
    constexpr explicit operator bool() const {
        return value != 0;
    }

    // 비교 연산자
    constexpr bool operator==(const ResourceID& other) const { return value == other.value; }
    constexpr bool operator!=(const ResourceID& other) const { return value != other.value; }
    constexpr bool operator<(const ResourceID& other) const { return value < other.value; }
};

// 5. 해시맵 지원 (std::hash 특수화)
namespace std {
    template <>
    struct hash<ResourceID> {
        std::size_t operator()(const ResourceID& id) const noexcept {
            return static_cast<std::size_t>(id.value);
        }
    };
}
#endif