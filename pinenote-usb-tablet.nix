{
  stdenv,
  pkg-config,
  libconfig,
  libevdev,
  libusbgx,
}:
stdenv.mkDerivation {
  name = "pinenote-usb-tablet";
  src = ./.;
  nativeBuildInputs = [ pkg-config ];
  buildInputs = [
    libconfig
    libevdev
    libusbgx
  ];
  installPhase = ''
    mkdir -p $out/bin
    cp pinenote-usb-tablet $out/bin/pinenote-usb-tablet
  '';
}
