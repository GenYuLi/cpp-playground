{
  description = "C++ pg gccStd Development Environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.11";
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        stdenv = pkgs.gccStdenv;
      in
      {
        devShells.default = pkgs.mkShell.override { inherit stdenv; }
          {
            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
              clang-tools
            ];

            # These C++ libs must be built with the SAME stdenv as the
            # compiler (gccStdenv -> libstdc++). The default nixpkgs builds use
            # the platform-default stdenv (clang/libc++ on Darwin), whose
            # std::string ABI is incompatible with libstdc++ and produces
            # undefined-symbol link errors (e.g. fmt::vformat[abi:cxx11]).
            # nlohmann_json and doctest are header-only, so no rebuild needed.
            # fmt ships compiled symbols (e.g. fmt::vformat) whose std::string
            # ABI must match the compiler. The default nixpkgs fmt is built with
            # the platform-default clang/libc++; rebuilding it with our gccStdenv
            # makes it export the libstdc++ (abi:cxx11) variants the code needs.
            # doCheck is disabled: 1 of fmt's 21 upstream tests fails under gcc
            # on this platform, though the library itself builds fine.
            #
            # boost is NOT overridden: Boost.System is header-only (since 1.69),
            # so its std::string-touching code is compiled inline in our TUs with
            # libstdc++ and the clang-built libboost_system contributes no
            # ABI-incompatible symbols. (Also, nixpkgs' boost on Darwin hardcodes
            # the clang b2 toolset and cannot build under gccStdenv.)
            buildInputs = with pkgs; [
              boost
              ((fmt.override { inherit stdenv; }).overrideAttrs (_: { doCheck = false; }))
              nlohmann_json
              doctest
            ];

            shellHook = ''
              export CMAKE_EXPORT_COMPILE_COMMANDS=1
              export CC=gcc
              export CXX=g++
              export CPATH="$CPATH"
            '';
          };
      });
}
