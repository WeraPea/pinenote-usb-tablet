{
  stdenv,
  pkg-config,
  libusbgx,
  libconfig,
}:
stdenv.mkDerivation {
  name = "pinenote-usb-tablet";
  src = ./.;
  nativeBuildInputs = [ pkg-config ];
  buildInputs = [
    libusbgx
    libconfig
  ];
  installPhase = ''
    mkdir -p $out/bin
    cp pinenote-usb-tablet $out/bin/pinenote-usb-tablet
  '';
}
