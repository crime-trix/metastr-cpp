#include <metastr/metastr.hpp>

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
    return metastr::encrypted_differs_from_literal<seed>("public helper");
}

} // namespace

int main()
{
    const std::vector<test_case> tests{
        {"char_roundtrip", char_roundtrip},
        {"wide_roundtrip", wide_roundtrip},
        {"encoded_data_differs", encoded_data_differs},
        {"same_text_different_sites", same_text_different_sites},
        {"view_size_excludes_terminator", view_size_excludes_terminator},
        {"empty_string_roundtrip", empty_string_roundtrip},
        {"long_string_roundtrip", long_string_roundtrip},
        {"public_difference_check", public_difference_check},
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
