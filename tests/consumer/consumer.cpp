#include <metastr/metastr.hpp>

#include <string_view>

int main()
{
    auto text = METASTR("installed package");
    return text.view() == "installed package" ? 0 : 1;
}
