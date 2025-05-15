{
  description = "A commandline tool for creating, inspecting and modifying BitTorrent metafiles.";

  inputs = {
    nixpkgs.url = "nixpkgs";
    flake-parts.url = "flake-parts";
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { flake-parts, ... }@inputs:
    let
      name = "torrenttools";
      makePkg =
        { lib
        , stdenv
        , fetchFromGitHub
        , bencode
        , catch2
        , cli11
        , cmake
        , ctre
        , expected-lite
        , fmt
        , gsl-lite
        , howard-hinnant-date
        , yaml-cpp
        , ninja
        , nlohmann_json
        , openssl
        , re2
        , sigslot
        , tbb
        }:

        stdenv.mkDerivation {
          pname = name;
          version = "0.6.2";
          nativeBuildInputs = [ cmake ninja ];
          buildInputs = [
            bencode
            catch2
            cli11
            ctre
            expected-lite
            fmt
            gsl-lite
            howard-hinnant-date
            yaml-cpp
            nlohmann_json
            openssl
            re2
            sigslot
            tbb
          ];
          src = with lib.fileset; toSource {
            root = ./.;
            fileset = fileFilter
              (file: ! (lib.elem file.name [ "flake.nix" "flake.lock" ]))
              ./.;
          };
          meta.mainProgram = name;
        };

      shellOverride = pkgs: oldAttrs: {
        name = "${name}-dev-shell";
        version = null;

        # https://github.com/NixOS/nixpkgs/issues/214945
        nativeBuildInputs = (oldAttrs.nativeBuildInputs or [ ]) ++ (with pkgs; [
          clang-tools
        ]);

        # make ninja output colorful
        shellHook = ''
          export NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -fdiagnostics-color=always"
        '';
      };

      overlay = final: prev: {
        ${name} = final.callPackage makePkg { };
      };

    in
    # flake-parts boilerplate
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [
        inputs.treefmt-nix.flakeModule
      ];

      flake.overlays.default = overlay;

      systems = inputs.nixpkgs.lib.systems.flakeExposed;

      perSystem = { system, config, pkgs, ... }: {
        packages.default = config.legacyPackages.${name};
        packages.${name} = config.packages.default;
        legacyPackages = pkgs;

        _module.args.pkgs = import inputs.nixpkgs {
          inherit system;
          overlays = [ overlay ];
        };

        devShells.default = config.packages.default.overrideAttrs (shellOverride pkgs);

        treefmt = {
          programs.clang-format.enable = true;
          programs.nixpkgs-fmt.enable = true;
        };
      };
    };
}
