#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

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
constexpr std::uint32_t format_version = 1;

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
    using unsigned_char = std::make_unsigned_t<Char>;

    unsigned_char value = 0;
    for (std::size_t byte = 0; byte < sizeof(Char); ++byte) {
        const auto stream_index = index * sizeof(Char) + byte;
        const auto word = stream_word(seed, stream_index / 8);
        const auto shift = static_cast<unsigned int>((stream_index % 8) * 8);
        const auto part = static_cast<unsigned_char>((word >> shift) & 0xff);
        value = static_cast<unsigned_char>(value | static_cast<unsigned_char>(part << (byte * 8)));
    }

    return static_cast<Char>(value);
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

template <class Char>
inline constexpr bool supported_character_v =
    std::is_same_v<Char, char> ||
    std::is_same_v<Char, wchar_t>
#if defined(__cpp_char8_t)
    || std::is_same_v<Char, char8_t>
#endif
    || std::is_same_v<Char, char16_t> ||
    std::is_same_v<Char, char32_t>;

template <class T>
void secure_zero(std::span<T> data) noexcept
{
    detail::secure_zero_bytes(data.data(), data.size_bytes());
}

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
    [[nodiscard]] Char* data() noexcept { return data_.data(); }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return N ? N - 1 : 0; }
    [[nodiscard]] constexpr std::size_t storage_size() const noexcept { return N; }
    [[nodiscard]] constexpr std::size_t byte_size() const noexcept { return N * sizeof(Char); }
    [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
    [[nodiscard]] std::basic_string_view<Char> view() const noexcept { return {data_.data(), size()}; }
    [[nodiscard]] std::span<const Char, N> storage() const noexcept { return std::span<const Char, N>(data_); }
    [[nodiscard]] std::span<Char, N> storage() noexcept { return std::span<Char, N>(data_); }
    [[nodiscard]] operator std::basic_string_view<Char>() const noexcept { return view(); }

    void clear() noexcept { wipe(); }

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

    [[nodiscard]] static constexpr std::uint64_t seed() noexcept { return Seed; }
    [[nodiscard]] static constexpr std::size_t size() noexcept { return N ? N - 1 : 0; }
    [[nodiscard]] static constexpr std::size_t storage_size() noexcept { return N; }
    [[nodiscard]] static constexpr std::size_t byte_size() noexcept { return N * sizeof(Char); }
    [[nodiscard]] static constexpr std::uint32_t version() noexcept { return format_version; }
    [[nodiscard]] constexpr std::basic_string_view<Char> encoded_view() const noexcept { return {data.data(), data.size()}; }

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

    [[nodiscard]] constexpr bool matches(const decoded_string<Char, N>& decoded) const noexcept
    {
        std::uint64_t current = Seed ^ static_cast<std::uint64_t>(N);
        for (std::size_t i = 0; i < N; ++i) {
            current ^= static_cast<std::uint64_t>(static_cast<std::make_unsigned_t<Char>>(decoded.storage()[i])) +
                detail::stream_word(Seed, i);
            current = detail::rotl64(current, 9);
        }

        return detail::mix64(current) == checksum;
    }

    bool decode_into(std::span<Char> output) const noexcept
    {
        if (output.size() < N) {
            return false;
        }

        for (std::size_t i = 0; i < N; ++i) {
            output[i] = static_cast<Char>(data[i] ^ detail::mask_for<Char>(Seed, i));
        }
        return true;
    }

    bool decode_into(std::span<Char, N> output) const noexcept
    {
        for (std::size_t i = 0; i < N; ++i) {
            output[i] = static_cast<Char>(data[i] ^ detail::mask_for<Char>(Seed, i));
        }
        return true;
    }

    [[nodiscard]] decoded_string<Char, N> decode() const noexcept
    {
        std::array<Char, N> out{};
        decode_into(std::span<Char, N>(out));
        return decoded_string<Char, N>(out);
    }

    template <class Fn>
    auto with_decoded(Fn&& fn) const
    {
        auto decoded = decode();
        if constexpr (std::is_void_v<std::invoke_result_t<Fn, std::basic_string_view<Char>>>) {
            std::forward<Fn>(fn)(decoded.view());
        } else {
            return std::forward<Fn>(fn)(decoded.view());
        }
    }
};

template <std::uint64_t Seed, class Char, std::size_t N>
[[nodiscard]] consteval auto make_blob(const Char (&literal)[N])
{
    static_assert(supported_character_v<Char>, "metastr supports char, wchar_t, char8_t, char16_t and char32_t literals");

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

#define METASTR_U8(str) ([] { \
    constexpr auto blob = ::metastr::make_blob<METASTR_SITE_SEED()>(str); \
    return blob.decode(); \
}())

#define METASTR_U16(str) ([] { \
    constexpr auto blob = ::metastr::make_blob<METASTR_SITE_SEED()>(str); \
    return blob.decode(); \
}())

#define METASTR_U32(str) ([] { \
    constexpr auto blob = ::metastr::make_blob<METASTR_SITE_SEED()>(str); \
    return blob.decode(); \
}())
