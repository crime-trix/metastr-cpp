#include <metastr/metastr.hpp>

#include <array>
#include <iostream>
#include <span>
#include <string_view>

int main()
{
    constexpr auto blob = metastr::make_blob<0xdecafbad10002000ull>("manual buffer");

    std::array<char, blob.storage_size()> output{};
    if (!blob.decode_into(std::span<char>(output.data(), output.size()))) {
        return 1;
    }

    std::cout << std::string_view(output.data(), blob.size()) << '\n';
    metastr::secure_zero(std::span<char>(output.data(), output.size()));
}
