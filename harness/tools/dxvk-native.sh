#!/bin/bash
# dxvk-native's Direct3D 9 for the Linux program (fnvd_linux): DXVK's own x86-64 Linux release build (Steam Runtime
# sniper; it needs glibc 2.27 or later; its SDL3 window-system path is chosen at run time by DXVK_WSI_DRIVER=SDL3),
# unpacked into env/cache/dxvk-native (or $1): lib/ (libdxvk_d3d9.so and its soname link libdxvk_d3d9.so.0) and
# include/ (dxvk's headers, for comparing src/SDK/DirectX/d3d9.h with them). CMake's FNVD_DXVK_DIR points at the folder.
# Usage: harness/tools/dxvk-native.sh [DIR]
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
version=3.1.1
sha256=506fa1ea5bb6ce9a89e11436f7ab5191bc0196e8a577e0cf0b16a45bbc9991b7
url=https://github.com/doitsujin/dxvk/releases/download/v$version/dxvk-native-$version-steamrt-sniper.tar.gz
dest=${1:-$root/env/cache/dxvk-native}
tarball=$root/env/cache/dxvk-native-$version-steamrt-sniper.tar.gz

if [[ ! -f $tarball ]] || ! echo "$sha256  $tarball" | sha256sum -c --quiet; then
    curl -fsSL -o "$tarball.part" "$url"
    echo "$sha256  $tarball.part" | sha256sum -c --quiet
    mv "$tarball.part" "$tarball"
fi
rm -rf "$dest.part"
mkdir -p "$dest.part"
tar -xzf "$tarball" -C "$dest.part" usr/lib usr/include/dxvk
mkdir -p "$dest.part/lib"
# Only Direct3D 9: the real file and its soname and link-time names.
cp -P "$dest.part"/usr/lib/libdxvk_d3d9.so* "$dest.part/lib/"
mv "$dest.part/usr/include/dxvk" "$dest.part/include"
rm -rf "$dest.part/usr"
echo "dxvk-native $version" > "$dest.part/VERSION"
rm -rf "$dest"
mv "$dest.part" "$dest"
echo "dxvk-native $version: $dest/lib/$(readlink "$dest/lib/libdxvk_d3d9.so.0")"
