# metastr-cpp

`metastr-cpp` is a small C++20 compile-time string encoding library for reducing plain string exposure in compiled binaries.

[![ci](https://github.com/crime-trix/metastr-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/crime-trix/metastr-cpp/actions/workflows/ci.yml)

It is not a cryptographic storage system. Use it for lightweight static string hiding, diagnostics tools, demos, or resource labels. Use a real envelope such as `metacrypt-cpp` for data that needs cryptographic confidentiality and integrity.

## Example

```cpp
#include <metastr/metastr.hpp>

auto text = METASTR("compile-time encoded string");
std::puts(text.c_str());
```

Wide literals are supported:

```cpp
auto text = METASTR_W(L"wide encoded string");
```

## Design

- `consteval` blob generation;
- per-call-site seed from file, line, and counter;
- no fixed maximum string length;
- `char` and `wchar_t` support;
- decoded owning buffer wipes itself on destruction;
- dependency-free C++20 header.

## Security Notes

This protects against simple static string extraction. It does not protect against a debugger, runtime memory inspection, emulator traces, or an attacker who can execute the binary and observe decoded strings.

## Build

```sh
cmake -S . -B build -DMETASTR_BUILD_EXAMPLES=ON -DMETASTR_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
