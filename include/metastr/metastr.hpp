#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace metastr {

constexpr std::uint64_t fnv_offset = 14695981039346656037ull;
constexpr std::uint64_t fnv_prime = 1099511628211ull;
constexpr std::uint64_t stream_constant = 0x9e3779b97f4a7c15ull;

namespace detail {

constexpr std::uint64_t rotl64(std::uint64_t value, unsigned int shift) noexcept
{
    shift &= 63;
    return (value << shift) | (value >> ((64 - shift) & 63));
}

constexpr std::uint64_t mix64(std::uint64_t value) noexcept
{
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ull;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebull;
    value ^= value >> 31;
    return value;
}

constexpr std::uint64_t hash_bytes(const char* data, std::size_t size) noexcept
{
    std::uint64_t hash = fnv_offset;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= static_cast<unsigned char>(data[i]);
        hash *= fnv_prime;
    }
    return hash;
}

constexpr std::uint64_t site_seed(const char* file, std::size_t file_size, int line, int counter) noexcept
{
    auto seed = hash_bytes(file, file_size);
    seed ^= static_cast<std::uint64_t>(line) * 0x100000001b3ull;
    seed ^= rotl64(static_cast<std::uint64_t>(counter) + stream_constant, 17);
    return mix64(seed);
}

constexpr std::uint64_t stream_word(std::uint64_t seed, std::uint64_t index) noexcept
{
    return mix64(seed + stream_constant * (index + 1));
}

template <class Char>
constexpr Char mask_for(std::uint64_t seed, std::size_t index) noexcept
{
    constexpr auto width = sizeof(Char) * 8;
    const auto word = stream_word(seed, index / 8);
    const auto byte_shift = static_cast<unsigned int>((index % 8) * 8);
    const auto mask = static_cast<unsigned char>((word >> byte_shift) & 0xff);

    if constexpr (sizeof(Char) == 1) {
        return static_cast<Char>(mask);
    } else {
        const auto hi = static_cast<unsigned char>((stream_word(seed ^ 0xa5a5a5a5a5a5a5a5ull, index / 8) >> byte_shift) & 0xff);
        return static_cast<Char>((static_cast<unsigned int>(hi) << (width / 2)) | mask);
    }
}

inline void secure_zero_bytes(void* data, std::size_t size) noexcept
{
    if (!data || !size) {
        return;
    }

#if defined(_WIN32)
    ::SecureZeroMemory(data, size);
#else
    volatile unsigned char* p = static_cast<volatile unsigned char*>(data);
    while (size--) {
        *p++ = 0;
    }
#endif
}

} // namespace detail

template <class Char, std::size_t N>
class decoded_string {
public:
    using value_type = Char;

    decoded_string() = default;

    explicit decoded_string(std::array<Char, N> data) noexcept : data_(data) {}

    ~decoded_string()
    {
        wipe();
    }

    decoded_string(const decoded_string&) = delete;
    decoded_string& operator=(const decoded_string&) = delete;

    decoded_string(decoded_string&& other) noexcept : data_(other.data_)
    {
        other.wipe();
    }

    decoded_string& operator=(decoded_string&& other) noexcept
    {
        if (this != &other) {
            wipe();
            data_ = other.data_;
            other.wipe();
        }
        return *this;
    }

    [[nodiscard]] const Char* c_str() const noexcept { return data_.data(); }
    [[nodiscard]] const Char* data() const noexcept { return data_.data(); }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return N ? N - 1 : 0; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
    [[nodiscard]] std::basic_string_view<Char> view() const noexcept { return {data_.data(), size()}; }
    [[nodiscard]] operator std::basic_string_view<Char>() const noexcept { return view(); }

private:
    void wipe() noexcept
    {
        detail::secure_zero_bytes(data_.data(), data_.size() * sizeof(Char));
    }

    std::array<Char, N> data_{};
};

template <class Char, std::size_t N, std::uint64_t Seed>
struct basic_blob {
    using value_type = Char;

    std::array<Char, N> data{};
    std::uint64_t checksum = 0;

    [[nodiscard]] constexpr bool contains_plaintext(std::basic_string_view<Char> plain) const noexcept
    {
        if (plain.size() + 1 != N) {
            return false;
        }

        for (std::size_t i = 0; i < plain.size(); ++i) {
            if (data[i] != plain[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] decoded_string<Char, N> decode() const noexcept
    {
        std::array<Char, N> out{};
        for (std::size_t i = 0; i < N; ++i) {
            out[i] = static_cast<Char>(data[i] ^ detail::mask_for<Char>(Seed, i));
        }
        return decoded_string<Char, N>(out);
    }
};

template <std::uint64_t Seed, class Char, std::size_t N>
[[nodiscard]] consteval auto make_blob(const Char (&literal)[N])
{
    static_assert(std::is_same_v<Char, char> || std::is_same_v<Char, wchar_t>, "metastr supports char and wchar_t literals");

    basic_blob<Char, N, Seed> blob{};
    std::uint64_t checksum = Seed ^ static_cast<std::uint64_t>(N);

    for (std::size_t i = 0; i < N; ++i) {
        blob.data[i] = static_cast<Char>(literal[i] ^ detail::mask_for<Char>(Seed, i));
        checksum ^= static_cast<std::uint64_t>(static_cast<std::make_unsigned_t<Char>>(literal[i])) + detail::stream_word(Seed, i);
        checksum = detail::rotl64(checksum, 9);
    }

    blob.checksum = detail::mix64(checksum);
    return blob;
}

template <std::uint64_t Seed, class Char, std::size_t N>
[[nodiscard]] consteval bool encrypted_differs_from_literal(const Char (&literal)[N])
{
    const auto blob = make_blob<Seed>(literal);
    for (std::size_t i = 0; i < N; ++i) {
        if (blob.data[i] == literal[i]) {
            return false;
        }
    }
    return true;
}

} // namespace metastr

#define METASTR_SITE_SEED() (::metastr::detail::site_seed(__FILE__, sizeof(__FILE__) - 1, __LINE__, __COUNTER__))

#define METASTR(str) ([] { \
    constexpr auto blob = ::metastr::make_blob<METASTR_SITE_SEED()>(str); \
    return blob.decode(); \
}())

#define METASTR_W(str) ([] { \
    constexpr auto blob = ::metastr::make_blob<METASTR_SITE_SEED()>(str); \
    return blob.decode(); \
}())
