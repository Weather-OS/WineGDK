{
  description = "WineGDK";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        inherit (pkgs) lib;

        versionFile = lib.trim (builtins.readFile ./VERSION);
        version = builtins.elemAt (builtins.match ".*([0-9]{2,}(\\.[0-9]+)+)$" versionFile) 0;

        src = ./.;
        pname = "wine-gdk";
      in
      {
        packages = rec {
          wine-gdk64 = pkgs.wine64Packages.base.overrideAttrs (p: {
            inherit
              src
              pname
              version
              ;
            patches = [ ];
          });

          default = wine-gdk64;
        };

        apps = rec {
          wine-gdk64 = {
            type = "app";
            program = "${self.packages.${system}.wine-gdk64}/bin/wine64";
          };
          default = wine-gdk64;
        };

        formatter = pkgs.nixfmt-tree;
      }
    );
}
