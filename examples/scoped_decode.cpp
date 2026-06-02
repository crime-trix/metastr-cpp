#include <metastr/metastr.hpp>

#include <iostream>
#include <string_view>

int main()
{
    constexpr auto label = metastr::make_blob<0x1234fedc98760011ull>("scoped decode");

    label.with_decoded([](std::string_view text) {
        std::cout << text << '\n';
    });
}
