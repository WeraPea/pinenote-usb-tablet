{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    systems.url = "github:nix-systems/default";
  };

  outputs =
    {
      self,
      nixpkgs,
      systems,
    }:
    let
      eachSystem = nixpkgs.lib.genAttrs (import systems);
      pkgsBySystem = eachSystem (
        system:
        import nixpkgs {
          inherit system;
        }
      );
    in
    {
      packages = eachSystem (system: rec {
        pinenote-usb-tablet = pkgsBySystem.${system}.callPackage ./pinenote-usb-tablet.nix { };
        default = pinenote-usb-tablet;
      });
      devShells = eachSystem (
        system:
        let
          pkgs = pkgsBySystem.${system};
        in
        {
          default = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
              evtest
              hid-tools
              libconfig
              libevdev
              libusbgx
              pkg-config
            ];
          };
        }
      );
    };
}
