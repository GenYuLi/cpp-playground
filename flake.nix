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
      in
      {
        devShells.default = pkgs.mkShell
          {
            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
              clang-tools
            ];

            buildInputs = with pkgs; [
              boost
              fmt
              nlohmann_json
            ] ++ (pkgs.lib.optionals pkgs.stdenv.isDarwin [
              pkgs.libiconv
              pkgs.darwin.apple_sdk.frameworks.CoreFoundation
            ]);

            shellHook = ''
              export CMAKE_EXPORT_COMPILE_COMMANDS=1
            '';
          };
      });
}
