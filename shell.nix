{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    cmake
    doxygen
    pandoc
    graphviz
    pkg-config
    libogg
    gcc
    gnumake
    ninja
    gdb
    valgrind
    clang-tools
    xxd
  ];

  CFLAGS = "-O3 -funroll-loops";
  CXXFLAGS = "-O3";

  shellHook = ''
    if [ ! -f "build/compile_commands.json" ]; then
      echo "Building compile_commands.json for LSP support..."
      mkdir -p build
      cd build
      cmake .. -DBUILD_SHARED_LIBS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -GNinja
      ninja -t compdb > compile_commands.json || true
      cd ..
    fi

    echo "FLAC development environment ready!"
    echo "To build:"
    echo "  mkdir build && cd build"
    echo "  cmake .. -DBUILD_SHARED_LIBS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -GNinja"
    echo "  ninja"
    echo ""
    echo "To run tests: ninja test"
  '';
}
