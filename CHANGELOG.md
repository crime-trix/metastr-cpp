# Changelog

## v0.3.4

- Added reproducible size benchmark executables for baseline, stream mode and automaton mode.
- Added a PowerShell benchmark runner that reports executable size, plaintext hit count and runtime hash exit codes.
- Added the benchmark runner to CI.

## v0.3.3

- Improved automaton state absorption for wide character types by feeding every encoded code-unit byte into the state transition.
- Added regression coverage for high-byte wide-character state changes.
- Added deterministic property-style roundtrip coverage across several seeds and string lengths.

## v0.3.2

- Updated the application example so protected sample values are not duplicated as ordinary input literals.

## v0.3.1

- Reduced public macro implementation duplication while preserving existing names.
- Added a CI binary scan that fails if protected example strings are present as plaintext in the Release executable.

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
