# metastr-cpp

`metastr-cpp` is a C++20 header-only library for compile-time string encoding and scoped runtime decode.

[![ci](https://github.com/crime-trix/metastr-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/crime-trix/metastr-cpp/actions/workflows/ci.yml)

The library is meant for reducing obvious string exposure in compiled binaries. It is not a cryptographic storage format and it does not claim to protect strings after they are decoded at runtime. Use `metacrypt-cpp` or another authenticated encryption scheme for data that needs real confidentiality and integrity.

## Features

- `consteval` literal encoding;
- stream and automaton-backed encoding modes;
- per-call-site seed material from file, line and counter;
- no fixed maximum string length;
- support for `char`, `wchar_t`, `char8_t`, `char16_t` and `char32_t`;
- move-only decoded buffers that wipe storage on destruction;
- scoped callback decode with `with_decoded`;
- manual decode into caller-provided buffers;
- encoded blob metadata: seed, size, byte size, checksum and format version;
- CMake target, install rules, examples and CI-tested test coverage.

## Quick Start

```cpp
#include <metastr/metastr.hpp>

#include <cstdio>

int main()
{
    auto text = METASTR("compile-time encoded string");
    std::puts(text.c_str());
}
```

Wide and UTF literals use dedicated macros:

```cpp
auto wide = METASTR_W(L"wide string");
auto utf8 = METASTR_U8(u8"utf8 string");
auto utf16 = METASTR_U16(u"utf16 string");
auto utf32 = METASTR_U32(U"utf32 string");
```

For a stateful finite-automaton transform, use the `AUTO` macros:

```cpp
auto text = METASTR_AUTO("encoded through a state machine");
auto wide = METASTR_AUTO_W(L"wide state machine string");
```

## Application Strings

Typical application use is intentionally small:

```cpp
#include <metastr/metastr.hpp>

#include <string_view>

bool login(std::string_view user, std::string_view password)
{
    const auto expected_user = METASTR("admin");
    const auto expected_password = METASTR_AUTO("change-me");

    return user == expected_user.view() && password == expected_password.view();
}
```

Use `METASTR` for the default compact transform. Use `METASTR_AUTO` when you want a stateful transform with per-character automaton state. Both are compile-time encoders and both decode only when the expression is evaluated.

## Scoped Decode

Use `with_decoded` when the plaintext should live only for the duration of a call:

```cpp
constexpr auto label = metastr::make_blob<0x1234fedc98760011ull>("scoped decode");

label.with_decoded([](std::string_view text) {
    std::puts(text.data());
});
```

The callback receives a `std::basic_string_view`. Do not store that view after the callback returns.

## Manual Buffers

For code that already owns storage, decode directly into a caller-provided span:

```cpp
constexpr auto blob = metastr::make_blob<0xdecafbad10002000ull>("manual buffer");

std::array<char, blob.storage_size()> output{};
if (blob.decode_into(std::span<char>(output.data(), output.size()))) {
    std::puts(output.data());
}

metastr::secure_zero(std::span<char>(output.data(), output.size()));
```

## API Notes

`METASTR(...)` is the convenient macro for normal use. It returns a move-only `decoded_string`.

`make_blob<Seed>(literal)` creates an encoded compile-time blob. This is useful when a stable seed is needed for tests, examples, or generated code.

`decoded_string` owns plaintext storage and wipes that storage when destroyed or reassigned.

`basic_blob::decode()` returns a `decoded_string`.

`basic_blob::decode_into(span)` writes decoded text plus the null terminator into an existing buffer.

`basic_blob::with_decoded(fn)` decodes, calls `fn(view)`, then wipes the temporary buffer when the call finishes.

## Security Model

This library protects against simple static string scans and low-effort extraction from binary data. It does not protect against:

- a debugger attached while the string is decoded;
- runtime memory inspection;
- tracing or emulating the decode routine;
- an attacker who can execute the binary and observe behavior;
- secrets that should never be shipped to the client.

Treat this as string hiding, not cryptography.

## Build

```sh
cmake -S . -B build -DMETASTR_BUILD_EXAMPLES=ON -DMETASTR_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Size Benchmark

The repository includes a small size benchmark with 40 identical string call-sites:

```powershell
./tools/run_size_bench.ps1
```

It builds three Release executables:

- `baseline`: plain string literals;
- `metastr_stream`: `METASTR`;
- `metastr_auto`: `METASTR_AUTO`.

The script reports executable size, delta from baseline, plaintext hit count for `api.secret.endpoint`, and the runtime hash exit code.
Plaintext hit count in the baseline can vary when the compiler/linker pools identical literals.

One MSVC `/O2 /GL /MT` run measured:

| Program | Size | Delta | Plaintext hits |
| --- | ---: | ---: | ---: |
| baseline | 110.0 KB | - | 40 |
| metastr_stream | 118.5 KB | +8.7 KB | 0 |
| metastr_auto | 124.0 KB | +14.3 KB | 0 |

Exact numbers vary by compiler, CRT, linker options and optimization level.

## Install

```sh
cmake --install build --config Release --prefix ./install
```

Then link the exported target:

```cmake
find_package(metastr-cpp CONFIG REQUIRED)
target_link_libraries(app PRIVATE metastr::metastr-cpp)
```

## License

MIT.
