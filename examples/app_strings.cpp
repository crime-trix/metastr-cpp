#include <metastr/metastr.hpp>

#include <iostream>
#include <string_view>

namespace {

bool login(std::string_view user, std::string_view password)
{
    const auto expected_user = METASTR("admin");
    const auto expected_password = METASTR_AUTO("change-me");

    return user == expected_user.view() && password == expected_password.view();
}

} // namespace

int main(int argc, char** argv)
{
    const auto window_title = METASTR_AUTO("Example Application");
    const auto log_prefix = METASTR("[metastr]");
    const auto user = argc > 1 ? std::string_view(argv[1]) : std::string_view();
    const auto password = argc > 2 ? std::string_view(argv[2]) : std::string_view();

    std::cout << window_title.c_str() << '\n';
    std::cout << log_prefix.c_str() << ' ' << (login(user, password) ? "ok" : "denied") << '\n';
}
