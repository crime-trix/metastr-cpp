# Changelog

## v0.3.0

- Added automaton-backed compile-time string blobs.
- Added `METASTR_AUTO`, `METASTR_AUTO_W`, `METASTR_AUTO_U8`, `METASTR_AUTO_U16` and `METASTR_AUTO_U32`.
- Added an application-style string example.
- Expanded tests for automaton decode, scoped callback decode and checksum verification.

## v0.2.0

- Added UTF literal support for `char8_t`, `char16_t` and `char32_t`.
- Added scoped callback decoding through `with_decoded`.
- Added manual decode into caller-provided spans.
- Added public `secure_zero`.
- Added blob metadata helpers and checksum verification.
- Added extra examples and broader tests.
- Added CMake install/export rules.
- Added an installed-package consumer check.

## v0.1.0

- Initial public release.
