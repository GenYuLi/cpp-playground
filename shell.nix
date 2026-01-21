{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    pkg-config
    boost

    # Python managed by uv, not Nix
    uv

    # Optional tools
    gdb
    clang-tools
  ];

  shellHook = ''
    echo "Welcome to the C++ Playground environment!"
    echo "Compiler: $(c++ --version | head -n1)"
    echo "uv: $(uv --version)"

    # Python environment is managed by .envrc and uv
    # Do NOT set PYTHONPATH here - it pollutes the venv
  '';
}
