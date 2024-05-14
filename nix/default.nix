{ self
, ...
}:

{
  # add all our packages based on host platform
  flake.overlays.default = _final: prev:
    let
      inherit (prev.stdenv.hostPlatform) system;
    in
    self.packages.${system};

  perSystem =
    { pkgs
    , self'
    , lib
    , ...
    }:
    let
      inherit (pkgs) callPackage;
    in
    {
      packages = {
        conan = callPackage ./conan.nix { };
      };
    };
}
