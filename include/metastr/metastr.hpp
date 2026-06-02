#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

namespace metastr {

constexpr std::uint64_t hash_basis = 0x6c8e9cf570932bd5ull;
constexpr std::uint64_t hash_multiplier = 0xda3e39cb94b95bdbull;
constexpr std::uint64_t stream_constant = 0x8f6d3b2a1c4e579bull;
constexpr std::uint64_t checksum_salt = 0xc27b9e3f4d1658a1ull;
constexpr std::uint64_t automaton_checksum_salt = 0x72f4d1a98c6e35b7ull;
constexpr std::uint32_t format_version = 1;

namespace detail {

constexpr std::uint64_t rotl64(std::uint64_t value, unsigned int shift) noexcept
{
    shift &= 63;
    return (value << shift) | (value >> ((64 - shift) & 63));
}

constexpr std::uint64_t mix64(std::uint64_t value) noexcept
{
    value ^= rotl64(value, 23) ^ (value >> 17);
    value *= 0xd6e8feb86659fd93ull;
    value ^= rotl64(value, 41) ^ (value >> 29);
    value *= 0xa5b85c5e198ed849ull;
    value ^= value >> 32;
    return value;
}

constexpr std::uint64_t hash_bytes(const char* data, std::size_t size) noexcept
{
    std::uint64_t hash = hash_basis ^ static_cast<std::uint64_t>(size);
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= static_cast<unsigned char>(data[i]) + static_cast<std::uint64_t>(i + 1) * 0x4f1bbcdc8f3a2d95ull;
        hash = rotl64(hash, 13);
        hash *= hash_multiplier;
    }
    return mix64(hash);
}

constexpr std::uint64_t site_seed(const char* file, std::size_t file_size, int line, int counter) noexcept
{
    auto seed = hash_bytes(file, file_size);
    seed ^= static_cast<std::uint64_t>(line) * 0xb17d2f4c6a9e31c3ull;
    seed ^= rotl64(static_cast<std::uint64_t>(counter) + stream_constant, 17);
    return mix64(seed);
}

constexpr std::uint64_t stream_word(std::uint64_t seed, std::uint64_t index) noexcept
{
    return mix64(seed + stream_constant * (index + 1));
}

constexpr std::uint8_t automaton_start(std::uint64_t seed) noexcept
{
    return static_cast<std::uint8_t>(mix64(seed ^ 0x91e10da5c79b4f27ull) & 0xff);
}

constexpr std::uint8_t automaton_step(std::uint64_t seed, std::uint8_t state, std::size_t index, std::uint8_t symbol) noexcept
{
    auto value = seed ^ (static_cast<std::uint64_t>(state) << 32);
    value ^= static_cast<std::uint64_t>(symbol) * 0xe4d2c6b91a7f538bull;
    value ^= static_cast<std::uint64_t>(index + 1) * stream_constant;
    return static_cast<std::uint8_t>(mix64(value) & 0xff);
}

constexpr std::uint8_t nonzero_byte(std::uint64_t word, std::size_t index) noexcept
{
    auto value = static_cast<std::uint8_t>((word >> ((index % 8) * 8)) & 0xff);
    if (value == 0) {
        value = static_cast<std::uint8_t>(0x5dU + static_cast<unsigned int>((index * 29U) & 0xffU));
        if (value == 0) {
            value = 0xa7;
        }
    }
    return value;
}

template <class Char>
constexpr std::uint8_t automaton_absorb(std::uint64_t seed, std::uint8_t state, std::size_t index, Char value) noexcept
{
    using unsigned_char = std::make_unsigned_t<Char>;

    auto current = static_cast<unsigned_char>(value);
    for (std::size_t byte = 0; byte < sizeof(Char); ++byte) {
        const auto part = static_cast<std::uint8_t>((current >> (byte * 8)) & 0xff);
        state = automaton_step(seed, state, index * sizeof(Char) + byte, part);
    }

    return state;
}

template <class Char>
constexpr Char automaton_mask_for(std::uint64_t seed, std::uint8_t state, std::size_t index) noexcept
{
    using unsigned_char = std::make_unsigned_t<Char>;

    unsigned_char value = 0;
    for (std::size_t byte = 0; byte < sizeof(Char); ++byte) {
        const auto stream_index = index * sizeof(Char) + byte;
        const auto word = stream_word(seed ^ (static_cast<std::uint64_t>(state) << 40), stream_index);
        const auto part = static_cast<unsigned_char>(nonzero_byte(word, stream_index));
        value = static_cast<unsigned_char>(value | static_cast<unsigned_char>(part << (byte * 8)));
    }

    return static_cast<Char>(value);
}

template <class Char>
constexpr Char mask_for(std::uint64_t seed, std::size_t index) noexcept
{
    using unsigned_char = std::make_unsigned_t<Char>;

    unsigned_char value = 0;
    for (std::size_t byte = 0; byte < sizeof(Char); ++byte) {
        const auto stream_index = index * sizeof(Char) + byte;
        const auto word = stream_word(seed, stream_index / 8);
        const auto part = static_cast<unsigned_char>(nonzero_byte(word, stream_index));
        value = static_cast<unsigned_char>(value | static_cast<unsigned_char>(part << (byte * 8)));
    }

    return static_cast<Char>(value);
}

inline void secure_zero_bytes(void* data, std::size_t size) noexcept
{
    if (!data || !size) {
        return;
    }

    volatile unsigned char* p = static_cast<volatile unsigned char*>(data);
    while (size--) {
        *p++ = 0;
    }
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

template <class Char, std::size_t N, std::uint64_t Seed>
struct automaton_blob {
    using value_type = Char;

    std::array<Char, N> data{};
    std::uint64_t checksum = 0;
    std::uint8_t initial_state = detail::automaton_start(Seed);

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
        std::uint64_t current = Seed ^ static_cast<std::uint64_t>(N) ^ checksum_salt;
        for (std::size_t i = 0; i < N; ++i) {
            current ^= static_cast<std::uint64_t>(static_cast<std::make_unsigned_t<Char>>(decoded.storage()[i])) +
                detail::stream_word(Seed ^ automaton_checksum_salt, i);
            current = detail::rotl64(current, 11);
        }

        return detail::mix64(current) == checksum;
    }

    bool decode_into(std::span<Char> output) const noexcept
    {
        if (output.size() < N) {
            return false;
        }

        auto state = initial_state;
        for (std::size_t i = 0; i < N; ++i) {
            output[i] = static_cast<Char>(data[i] ^ detail::automaton_mask_for<Char>(Seed, state, i));
            state = detail::automaton_absorb(Seed, state, i, data[i]);
        }
        return true;
    }

    bool decode_into(std::span<Char, N> output) const noexcept
    {
        return decode_into(std::span<Char>(output.data(), output.size()));
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
[[nodiscard]] consteval auto make_automaton_blob(const Char (&literal)[N])
{
    static_assert(supported_character_v<Char>, "metastr supports char, wchar_t, char8_t, char16_t and char32_t literals");

    automaton_blob<Char, N, Seed> blob{};
    auto state = blob.initial_state;
    std::uint64_t checksum = Seed ^ static_cast<std::uint64_t>(N) ^ checksum_salt;

    for (std::size_t i = 0; i < N; ++i) {
        blob.data[i] = static_cast<Char>(literal[i] ^ detail::automaton_mask_for<Char>(Seed, state, i));
        state = detail::automaton_absorb(Seed, state, i, blob.data[i]);
        checksum ^= static_cast<std::uint64_t>(static_cast<std::make_unsigned_t<Char>>(literal[i])) +
            detail::stream_word(Seed ^ automaton_checksum_salt, i);
        checksum = detail::rotl64(checksum, 11);
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

template <std::uint64_t Seed, class Char, std::size_t N>
[[nodiscard]] consteval bool automaton_encrypted_differs_from_literal(const Char (&literal)[N])
{
    const auto blob = make_automaton_blob<Seed>(literal);
    for (std::size_t i = 0; i < N; ++i) {
        if (blob.data[i] == literal[i]) {
            return false;
        }
    }
    return true;
}

} // namespace metastr

#define METASTR_SITE_SEED() (::metastr::detail::site_seed(__FILE__, sizeof(__FILE__) - 1, __LINE__, __COUNTER__))

#define METASTR_DETAIL_DECODE(str) ([] { \
    constexpr auto seed = METASTR_SITE_SEED(); \
    static_assert(::metastr::encrypted_differs_from_literal<seed>(str)); \
    constexpr auto blob = ::metastr::make_blob<seed>(str); \
    return blob.decode(); \
}())

#define METASTR_DETAIL_DECODE_AUTO(str) ([] { \
    constexpr auto seed = METASTR_SITE_SEED(); \
    static_assert(::metastr::automaton_encrypted_differs_from_literal<seed>(str)); \
    constexpr auto blob = ::metastr::make_automaton_blob<seed>(str); \
    return blob.decode(); \
}())

#define METASTR(str) METASTR_DETAIL_DECODE(str)
#define METASTR_W(str) METASTR_DETAIL_DECODE(str)
#define METASTR_U8(str) METASTR_DETAIL_DECODE(str)
#define METASTR_U16(str) METASTR_DETAIL_DECODE(str)
#define METASTR_U32(str) METASTR_DETAIL_DECODE(str)

#define METASTR_AUTO(str) METASTR_DETAIL_DECODE_AUTO(str)
#define METASTR_AUTO_W(str) METASTR_DETAIL_DECODE_AUTO(str)
#define METASTR_AUTO_U8(str) METASTR_DETAIL_DECODE_AUTO(str)
#define METASTR_AUTO_U16(str) METASTR_DETAIL_DECODE_AUTO(str)
#define METASTR_AUTO_U32(str) METASTR_DETAIL_DECODE_AUTO(str)
