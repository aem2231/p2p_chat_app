{
  description = "A Nix-flake-based C/C++ development environment with FTXUI";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    ftxui.url   = "github:ArthurSonzogni/FTXUI";
  };

  outputs = { self, nixpkgs, ftxui, ... }:

  let
    supportedSystems = [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ];

    # Helper to generate attributes per system
    forEachSupportedSystem = f:
      nixpkgs.lib.genAttrs supportedSystems (
        system:
          f {
            pkgs = import nixpkgs { inherit system; };
            system = system;
          }
      );
  in
  {
    devShells = forEachSupportedSystem (
      { pkgs, system }:
      {
        default = pkgs.mkShell {
          packages = with pkgs; [
            gcc
            clang-tools
            cmake
            codespell
            conan
            cppcheck
            doxygen
            gtest
            lcov
            vcpkg
            vcpkg-tool
            valgrind
            boost
            openssl
            fmt
            spdlog
            bear
            tokei
	    tree-sitter
          ]
          ++ (if system == "aarch64-darwin" then [] else [ gdb ])
          ++ [ ftxui.packages.${system}.ftxui ];

          shellHook = let
            ftxui_pkg = ftxui.packages.${system}.ftxui;
          in ''
            export CPATH="${ftxui_pkg}/include:$CPATH"
            export LIBRARY_PATH="${ftxui_pkg}/lib:$LIBRARY_PATH"
            export LD_LIBRARY_PATH="${ftxui_pkg}/lib:$LD_LIBRARY_PATH"
            export PKG_CONFIG_PATH="${ftxui_pkg}/lib/pkgconfig:$PKG_CONFIG_PATH"
          '';
        };
      }
    );
  };
}
