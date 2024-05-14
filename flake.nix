{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
    openmp-sdk.url = "github:ChronosXYZ/open.mp-sdk";
  };

  outputs =
    inputs@{ flake-parts
    , ...
    }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
      ];

      imports = [
        ./nix
      ];

      perSystem = { pkgs, lib, system, self', ... }: {
        devShells.default = pkgs.mkShell.override { stdenv = pkgs.multiStdenv; }
          (with pkgs;
          {
            packages = [
              # gccMultiStdenv.cc.cc.lib
              pkg-config
              gcc_multi
              python3
              cmake
              ninja

              self'.packages.conan
              inputs.openmp-sdk.packages.${system}.default
            ];
            # inputsFrom = [
            #   inputs.openmp-sdk.packages.${system}.default
            # ];
            # env.LD_LIBRARY_PATH = "${lib.makeLibraryPath gcc_multi}:${LD_LIBRARY_PATH}";
          });
      };
    };
}
