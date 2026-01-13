{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    pkg-config
    boost

    # Optional tools
    gdb
    clang-tools
  ];

  shellHook = ''
    echo "Welcome to the C++ Playground environment!"
    echo "Compiler: $(c++ --version | head -n1)"
  '';
}
