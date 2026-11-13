#!/bin/bash
# Build QEMU 11.1.3 (x86_64 system emulator only) with the BT-flags fix that
# OpenVMS x86-64 needs under TCG emulation. Takes ~5-10 min on an M-series Mac.
# Result: ./qemu/build/qemu-system-x86_64
set -e
cd "$(dirname "$0")"
for f in qemu ninja meson pkgconf glib pixman; do brew list "$f" >/dev/null 2>&1 || brew install "$f"; done
if [ ! -d qemu ]; then
  git clone --depth 1 --branch v11.1.3 https://gitlab.com/qemu-project/qemu.git qemu
  (cd qemu && git apply ../bt-flags.patch)
fi
mkdir -p qemu/build && cd qemu/build
[ -f build.ninja ] || ../configure --target-list=x86_64-softmmu --disable-docs --disable-werror
ninja qemu-system-x86_64
echo; echo "Built: $(pwd)/qemu-system-x86_64"; ./qemu-system-x86_64 --version | head -1
