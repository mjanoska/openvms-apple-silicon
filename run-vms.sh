#!/bin/bash
# Run the VSI OpenVMS x86-64 community kit on Apple Silicon with the patched QEMU.
#   Put your kit's  *-flat.vmdk  (from the VSI community package) in this folder.
#   Console = this Terminal. At the Boot Manager prompt type: BOOT
#   Clean stop: in VMS  @SYS$SYSTEM:SHUTDOWN , then Ctrl-A X here.
#   SSH once booted:  ssh -p 2222 system@localhost
#   Options:  CPUS=4 MEM=8192 ./run-vms.sh   (tested: 1-4 CPUs, 2-12 GB)
set -e
D="$(cd "$(dirname "$0")" && pwd)"; cd "$D"
Q="$D/qemu/build/qemu-system-x86_64"
FW="$(brew --prefix qemu)/share/qemu"
BASE="$(ls *-flat.vmdk 2>/dev/null | head -1)"
DISK=vms-system.qcow2; VARS=vms-vars.fd
[ -x "$Q" ] || { echo "Patched QEMU missing - run ./build-qemu.sh first"; exit 1; }
if [ ! -f "$DISK" ]; then
  [ -n "$BASE" ] || { echo "No *-flat.vmdk found - copy your VSI community kit disk here"; exit 1; }
  echo "Creating $DISK on top of $BASE (the kit file itself stays untouched)"
  qemu-img create -q -f qcow2 -b "$BASE" -F raw "$DISK"
fi
DATA=vms-data.qcow2   # your files - survives the yearly kit change
[ -f "$DATA" ] || qemu-img create -q -f qcow2 "$DATA" 8G
[ -f "$VARS" ] || cp "$FW/edk2-i386-vars.fd" "$VARS"
exec caffeinate -ims "$Q" -machine q35 -accel tcg -cpu Haswell-noTSX-IBRS \
  -smp "${CPUS:-4}" -m "${MEM:-8192}" \
  -drive if=pflash,format=raw,readonly=on,file="$FW/edk2-x86_64-code.fd" \
  -drive if=pflash,format=raw,file="$VARS" \
  -drive file="$DISK",if=none,id=d0,format=qcow2 -device ide-hd,drive=d0,bus=ide.0 \
  -drive file="$DATA",if=none,id=d1,format=qcow2 -device ide-hd,drive=d1,bus=ide.1 \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:2222-:22 -device e1000,netdev=n0 \
  -vga none -display none -serial mon:stdio
