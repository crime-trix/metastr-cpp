#pragma once

#include <cstdint>
#include <string_view>

namespace bench {

inline std::uint64_t fnv1a(std::uint64_t hash, std::string_view text) noexcept
{
    for (const auto ch : text) {
        hash ^= static_cast<unsigned char>(ch) + 0x7b;
        hash = (hash << 7) | (hash >> 57);
        hash *= 0xb4f0c9d23a7e5d8bull;
    }
    return hash;
}

inline int fold_exit_code(std::uint64_t hash) noexcept
{
    return static_cast<int>((hash >> 32) ^ (hash & 0xffffffffull));
}

} // namespace bench
