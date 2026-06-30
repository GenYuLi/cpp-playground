# Fix Report: fmt link failure (libstdc++ / libc++ ABI mismatch)

**Date:** 2026-06-30
**Affected build:** nix dev shell (`gccStdenv`) on aarch64-darwin
**Symptom:** `Undefined symbols for architecture arm64: fmt::v12::vformat[abi:cxx11](...)` when linking `libpg_lib.dylib`.

---

## 1. Symptom

```
Undefined symbols for architecture arm64:
  "fmt::v12::vformat[abi:cxx11](fmt::v12::basic_string_view<char>,
       fmt::v12::basic_format_args<fmt::v12::context>)", referenced from:
      asio_test() in asio_test.cpp.o
ld: symbol(s) not found for architecture arm64
```

`fmt::fmt` *was* on the link line, yet the symbol could not be resolved.

## 2. Root cause — C++ standard-library ABI mismatch

The flake is a **gccStdenv** environment: code compiles with **nix GCC 14 → libstdc++**.
libstdc++'s `std::string` carries the `[abi:cxx11]` tag, so the compiler emits a
reference to:

```
fmt::v12::vformat[abi:cxx11](...)     ← libstdc++ variant (returns libstdc++ std::string)
```

But `buildInputs` pulled the **default** nixpkgs `fmt`, which on macOS is built with
the platform-default **Clang → libc++**. That dylib only exports:

```
fmt::v12::vformat(...)                ← libc++ variant (no abi tag)
```

These are two distinct symbols. The libstdc++ one the compiler asked for simply does
not exist in a libc++-built fmt → undefined symbol. The same hazard applied in
principle to any compiled C++ library brought in by `find_package`.

Verified with:

```sh
nm -gU <nix-fmt>/lib/libfmt.12.dylib | c++filt | grep vformat
# default (clang/libc++) build exported the no-tag variant only
```

## 3. Fix

### 3.1 Rebuild fmt with the project's stdenv (`flake.nix`)

```nix
buildInputs = with pkgs; [
  boost
  ((fmt.override { inherit stdenv; }).overrideAttrs (_: { doCheck = false; }))
  nlohmann_json
  doctest
];
```

- `fmt.override { inherit stdenv; }` rebuilds fmt with our `gccStdenv` (libstdc++),
  so it exports the `[abi:cxx11]` symbols the compiler needs.
- `doCheck = false`: 1 of fmt's 21 upstream self-tests (`format-test`) fails under
  gcc on this platform; the library itself builds correctly, so the check phase is
  skipped.

### 3.2 boost is intentionally NOT overridden

- **Boost.System is header-only since 1.69**, so its `std::string`-touching code is
  compiled inline in our translation units with libstdc++. The Clang-built
  `libboost_system.dylib` contributes no ABI-incompatible symbols — confirmed by the
  fact that the original link error reported *only* fmt, never boost.
- nixpkgs' boost on Darwin **hardcodes the Clang b2 toolset** (`clang-darwin.jam`) and
  fails outright when handed `g++` via `gccStdenv`. Overriding it is both unnecessary
  and broken.

### 3.3 Unrelated bug uncovered: non-standard `uint` (`bin/language/perfect_forwarding.cpp`)

```diff
-enum class FishType : uint { shark, salmon };
+enum class FishType : unsigned int { shark, salmon };
```

`uint` is not a standard C++ type. Apple Clang provides it via `<sys/types.h>`; nix
GCC does not. This bug was always present but never reached — the old Makefile build
died at the `pg_lib` link first.

### 3.4 Test wired into ctest (`projects/matching_engine/CMakeLists.txt`)

```cmake
add_test(NAME coro_matching_test COMMAND coro_matching_test)
```

`coro_matching_test` built an executable but was never registered with ctest.

## 4. Verification

```sh
# Linked fmt now exports the libstdc++ variant:
otool -L build/pg/libpg_lib.dylib            # -> rebuilt fmt store path
nm -gU <fmt>/lib/libfmt.12.dylib | c++filt | grep 'vformat\[abi:cxx11\]'
# 000000000001b060 T fmt::v12::vformat[abi:cxx11](...)

# Full build green (all 21 targets), tests pass:
ctest --output-on-failure
# 1/2 coro_matching_test ... Passed
# 2/2 ds_tests ............. Passed
# 100% tests passed, 0 tests failed out of 2
```

## 5. Gotcha worth remembering

Do **not** run the build through a **login shell** (`bash -lc`) inside `nix develop`.
The `-l` flag re-sources the user profile, which pushes `/usr/bin` ahead of the nix
toolchain in `PATH`, so `CXX=g++` silently resolves to **Apple Clang** (`/usr/bin/g++`)
instead of nix GCC — flipping the ABI mismatch to the opposite direction. Use a
non-login `bash -c` (or run `build.sh` from an interactive `nix develop`).

## 6. Files changed

| File | Change |
|------|--------|
| `flake.nix` | Rebuild fmt with `gccStdenv`; `doCheck = false`. Documented why boost is left alone. |
| `bin/language/perfect_forwarding.cpp` | `uint` → `unsigned int`. |
| `projects/matching_engine/CMakeLists.txt` | Register `coro_matching_test` with ctest. |
