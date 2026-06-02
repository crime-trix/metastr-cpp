#include <metastr/metastr.hpp>

#include <array>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

struct test_case {
    const char* name;
    bool (*run)();
};

bool char_roundtrip()
{
    auto decoded = METASTR("hello metastr");
    return decoded.view() == "hello metastr";
}

bool wide_roundtrip()
{
    auto decoded = METASTR_W(L"hello wide metastr");
    return decoded.view() == L"hello wide metastr";
}

bool automaton_char_roundtrip()
{
    auto decoded = METASTR_AUTO("hello automaton metastr");
    return decoded.view() == "hello automaton metastr";
}

bool automaton_wide_roundtrip()
{
    auto decoded = METASTR_AUTO_W(L"wide automaton metastr");
    return decoded.view() == L"wide automaton metastr";
}

bool utf16_roundtrip()
{
    auto decoded = METASTR_U16(u"hello utf16 metastr");
    return decoded.view() == u"hello utf16 metastr";
}

bool utf32_roundtrip()
{
    auto decoded = METASTR_U32(U"hello utf32 metastr");
    return decoded.view() == U"hello utf32 metastr";
}

#if defined(__cpp_char8_t)
bool utf8_roundtrip()
{
    auto decoded = METASTR_U8(u8"hello utf8 metastr");
    return decoded.view() == u8"hello utf8 metastr";
}
#endif

bool encoded_data_differs()
{
    constexpr auto seed = 0x123456789abcdef0ull;
    constexpr auto blob = metastr::make_blob<seed>("static string");
    static_assert(!blob.contains_plaintext("static string"));
    return !blob.contains_plaintext("static string");
}

bool same_text_different_sites()
{
    constexpr auto a = metastr::make_blob<metastr::detail::site_seed("a.cpp", 5, 10, 1)>("same");
    constexpr auto b = metastr::make_blob<metastr::detail::site_seed("a.cpp", 5, 11, 1)>("same");
    return a.data != b.data;
}

bool view_size_excludes_terminator()
{
    auto decoded = METASTR("size");
    return decoded.size() == 4 && decoded.view().size() == 4;
}

bool empty_string_roundtrip()
{
    auto decoded = METASTR("");
    return decoded.empty() && decoded.view().empty() && decoded.c_str()[0] == '\0';
}

bool long_string_roundtrip()
{
    auto decoded = METASTR("metastr keeps literals encoded at compile time without imposing a tiny fixed maximum length");
    return decoded.view() == "metastr keeps literals encoded at compile time without imposing a tiny fixed maximum length";
}

bool public_difference_check()
{
    constexpr auto seed = 0x77ddaa5500114499ull;
    static_assert(metastr::encrypted_differs_from_literal<seed>("public helper"));
    static_assert(metastr::automaton_encrypted_differs_from_literal<seed>("public helper"));
    return metastr::encrypted_differs_from_literal<seed>("public helper") &&
        metastr::automaton_encrypted_differs_from_literal<seed>("public helper");
}

bool decode_into_buffer()
{
    constexpr auto blob = metastr::make_blob<0x1020304050607080ull>("buffered");
    std::array<char, blob.storage_size()> output{};
    return blob.decode_into(std::span<char>(output.data(), output.size())) &&
        std::string_view(output.data(), blob.size()) == "buffered";
}

bool automaton_decode_into_buffer()
{
    constexpr auto blob = metastr::make_automaton_blob<0x33445566778899aaull>("auto buffered");
    std::array<char, blob.storage_size()> output{};
    return blob.decode_into(std::span<char>(output.data(), output.size())) &&
        std::string_view(output.data(), blob.size()) == "auto buffered";
}

bool automaton_payload_differs_from_stream_payload()
{
    constexpr auto seed = 0x4242424242424242ull;
    constexpr auto stream_blob = metastr::make_blob<seed>("same input");
    constexpr auto auto_blob = metastr::make_automaton_blob<seed>("same input");

    auto stream_decoded = stream_blob.decode();
    auto auto_decoded = auto_blob.decode();

    return stream_blob.data != auto_blob.data &&
        stream_decoded.view() == "same input" &&
        auto_decoded.view() == "same input";
}

bool automaton_wide_state_uses_high_bytes()
{
    constexpr auto seed = 0x13579bdf2468ace0ull;
    constexpr auto a = metastr::make_automaton_blob<seed>(u"\u0101x");
    constexpr auto b = metastr::make_automaton_blob<seed>(u"\u0201x");

    auto decoded_a = a.decode();
    auto decoded_b = b.decode();

    return a.data[1] != b.data[1] &&
        decoded_a.view() == u"\u0101x" &&
        decoded_b.view() == u"\u0201x";
}

bool masks_do_not_emit_zero_bytes()
{
    constexpr auto seed = 0x74c2b8e19d3f560aull;

    for (std::size_t i = 0; i < 512; ++i) {
        const auto stream_mask = static_cast<unsigned char>(metastr::detail::mask_for<char>(seed, i));
        const auto auto_mask = static_cast<unsigned char>(metastr::detail::automaton_mask_for<char>(seed, 0x5a, i));
        if (stream_mask == 0 || auto_mask == 0) {
            return false;
        }
    }

    for (std::size_t i = 0; i < 128; ++i) {
        const auto stream_mask = static_cast<std::uint16_t>(metastr::detail::mask_for<char16_t>(seed, i));
        const auto auto_mask = static_cast<std::uint16_t>(metastr::detail::automaton_mask_for<char16_t>(seed, 0x9c, i));
        if ((stream_mask & 0x00ffU) == 0 || (stream_mask & 0xff00U) == 0 ||
            (auto_mask & 0x00ffU) == 0 || (auto_mask & 0xff00U) == 0) {
            return false;
        }
    }

    return true;
}

bool decode_into_rejects_small_buffer()
{
    constexpr auto blob = metastr::make_blob<0x8877665544332211ull>("small");
    std::array<char, 2> output{};
    return !blob.decode_into(std::span<char>(output.data(), output.size()));
}

bool checksum_matches_decoded_value()
{
    constexpr auto blob = metastr::make_blob<0x0badf00d12345678ull>("checked");
    auto decoded = blob.decode();
    return blob.matches(decoded);
}

bool automaton_checksum_matches_decoded_value()
{
    constexpr auto blob = metastr::make_automaton_blob<0x1122334455667788ull>("auto checked");
    auto decoded = blob.decode();
    return blob.matches(decoded);
}

bool scoped_callback_decode()
{
    constexpr auto blob = metastr::make_blob<0xaabbccddeeff0011ull>("scoped");
    bool seen = false;

    blob.with_decoded([&](std::string_view text) {
        seen = text == "scoped";
    });

    const auto size = blob.with_decoded([](std::string_view text) {
        return text.size();
    });

    return seen && size == 6;
}

bool automaton_scoped_callback_decode()
{
    constexpr auto blob = metastr::make_automaton_blob<0x9988776655443322ull>("auto scoped");
    return blob.with_decoded([](std::string_view text) {
        return text == "auto scoped";
    });
}

bool deterministic_property_roundtrips()
{
    constexpr auto stream_empty = metastr::make_blob<0x0000000000000001ull>("");
    constexpr auto auto_empty = metastr::make_automaton_blob<0x0000000000000001ull>("");
    constexpr auto stream_one = metastr::make_blob<0x0123456789abcdefull>("a");
    constexpr auto auto_one = metastr::make_automaton_blob<0x0123456789abcdefull>("a");
    constexpr auto stream_short = metastr::make_blob<0xfedcba9876543210ull>("short");
    constexpr auto auto_short = metastr::make_automaton_blob<0xfedcba9876543210ull>("short");
    constexpr auto stream_text = metastr::make_blob<0x52d6a3f8b91c047eull>("with spaces and punctuation !?");
    constexpr auto auto_text = metastr::make_automaton_blob<0x52d6a3f8b91c047eull>("with spaces and punctuation !?");
    constexpr auto stream_long = metastr::make_blob<0xc8f14b6d2e903a75ull>("0123456789abcdef0123456789abcdef0123456789abcdef");
    constexpr auto auto_long = metastr::make_automaton_blob<0xc8f14b6d2e903a75ull>("0123456789abcdef0123456789abcdef0123456789abcdef");

    auto stream_empty_decoded = stream_empty.decode();
    auto auto_empty_decoded = auto_empty.decode();
    auto stream_one_decoded = stream_one.decode();
    auto auto_one_decoded = auto_one.decode();
    auto stream_short_decoded = stream_short.decode();
    auto auto_short_decoded = auto_short.decode();
    auto stream_text_decoded = stream_text.decode();
    auto auto_text_decoded = auto_text.decode();
    auto stream_long_decoded = stream_long.decode();
    auto auto_long_decoded = auto_long.decode();

    return stream_empty_decoded.view() == "" &&
        auto_empty_decoded.view() == "" &&
        stream_one_decoded.view() == "a" &&
        auto_one_decoded.view() == "a" &&
        stream_short_decoded.view() == "short" &&
        auto_short_decoded.view() == "short" &&
        stream_text_decoded.view() == "with spaces and punctuation !?" &&
        auto_text_decoded.view() == "with spaces and punctuation !?" &&
        stream_long_decoded.view() == "0123456789abcdef0123456789abcdef0123456789abcdef" &&
        auto_long_decoded.view() == "0123456789abcdef0123456789abcdef0123456789abcdef" &&
        stream_empty.matches(stream_empty_decoded) &&
        auto_empty.matches(auto_empty_decoded) &&
        stream_one.matches(stream_one_decoded) &&
        auto_one.matches(auto_one_decoded) &&
        stream_short.matches(stream_short_decoded) &&
        auto_short.matches(auto_short_decoded) &&
        stream_text.matches(stream_text_decoded) &&
        auto_text.matches(auto_text_decoded) &&
        stream_long.matches(stream_long_decoded) &&
        auto_long.matches(auto_long_decoded);
}

} // namespace

int main()
{
    const std::vector<test_case> tests{
        {"char_roundtrip", char_roundtrip},
        {"wide_roundtrip", wide_roundtrip},
        {"automaton_char_roundtrip", automaton_char_roundtrip},
        {"automaton_wide_roundtrip", automaton_wide_roundtrip},
        {"utf16_roundtrip", utf16_roundtrip},
        {"utf32_roundtrip", utf32_roundtrip},
#if defined(__cpp_char8_t)
        {"utf8_roundtrip", utf8_roundtrip},
#endif
        {"encoded_data_differs", encoded_data_differs},
        {"same_text_different_sites", same_text_different_sites},
        {"view_size_excludes_terminator", view_size_excludes_terminator},
        {"empty_string_roundtrip", empty_string_roundtrip},
        {"long_string_roundtrip", long_string_roundtrip},
        {"public_difference_check", public_difference_check},
        {"decode_into_buffer", decode_into_buffer},
        {"automaton_decode_into_buffer", automaton_decode_into_buffer},
        {"automaton_payload_differs_from_stream_payload", automaton_payload_differs_from_stream_payload},
        {"automaton_wide_state_uses_high_bytes", automaton_wide_state_uses_high_bytes},
        {"masks_do_not_emit_zero_bytes", masks_do_not_emit_zero_bytes},
        {"decode_into_rejects_small_buffer", decode_into_rejects_small_buffer},
        {"checksum_matches_decoded_value", checksum_matches_decoded_value},
        {"automaton_checksum_matches_decoded_value", automaton_checksum_matches_decoded_value},
        {"scoped_callback_decode", scoped_callback_decode},
        {"automaton_scoped_callback_decode", automaton_scoped_callback_decode},
        {"deterministic_property_roundtrips", deterministic_property_roundtrips},
    };

    for (const auto& test : tests) {
        const bool ok = test.run();
        std::cout << (ok ? "[pass] " : "[fail] ") << test.name << '\n';
        if (!ok) {
            return 1;
        }
    }

    std::cout << tests.size() << " MetaStr test cases passed\n";
}
