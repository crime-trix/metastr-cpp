#include <metastr/metastr.hpp>

#include <iostream>

int main()
{
    auto text = METASTR("compile-time encoded string");
    auto wide = METASTR_W(L"wide encoded string");

    std::cout << text.c_str() << '\n';
    std::wcout << wide.c_str() << L'\n';
}
