# OpenVMS x86-64 on Apple Silicon

Runs the **VSI OpenVMS x86-64 community kit** (tested: V9.2-3, 2026 kit) on an
Apple Silicon Mac (tested: M4 MacBook Pro) using QEMU's x86 emulation (TCG),
with a small QEMU fix. Full boot to the login prompt takes about 1 minute
with 4 CPUs / 8 GB.

## Why a patched QEMU?

Stock QEMU (tested up to 11.1.3) gets OpenVMS through the Boot Manager and the
kernel, but SYSINIT then dies at `%SYSINIT-I-RECERL`:

    %RMS-F-BUG, fatal RMS condition (FFFFFFE5), process deleted
    Process creation failed after checkpoint: 1027

and the console falls silent after `%SYSTEM-W-NOPAGEFILE`.

Cause: QEMU's emulation of the x86 `BT` (bit test) instruction rewrites the
OF flag. Intel documents OF/SF/AF/PF as "undefined" after BT, but real Intel
and AMD CPUs leave them unchanged - and VSI's VAX-MACRO-to-x86 compiler relies
on that. It translates VAX `SOBGTR` as

    decl  %r10d          ; counter - 1, sets ZF/SF/OF
    btw   $8, %r15w      ; restore the VAX C bit into CF
    jg    loop           ; loop while counter > 0

Under QEMU the `jg` stays taken for negative counters, so an RMS loop never
ends, allocates buffers until it runs out of address space, and RMS bugchecks.

`bt-flags.patch` makes QEMU always materialise EFLAGS and replace only CF for
BT/BTS/BTR/BTC (the path QEMU already used in other cases), matching hardware.

Verified with a small test (dec from 0, then `btw $8`, then read flags):

| CPU                       | OF after btw | jg       |
|---------------------------|--------------|----------|
| Intel Xeon (hardware)     | 0            | not taken|
| QEMU 8.2 / 11.1.3 stock   | 1            | TAKEN    |
| QEMU + bt-flags.patch     | 0            | not taken|

## Use

1. Get the VSI community licence kit (vmssoftware.com, community licence
   program). Copy its `*-flat.vmdk` (and the small descriptor `.vmdk`) into
   this folder. Do **not** redistribute it - it is licensed to you.
2. `./build-qemu.sh`  (once; needs Homebrew)
3. `./run-vms.sh`     (console in this Terminal; type `BOOT` at the Boot Manager)
4. From another Terminal: `ssh -p 2222 system@localhost`
5. Clean stop: `$ @SYS$SYSTEM:SHUTDOWN` in VMS, then Ctrl-A X.

Your changes live in `vms-system.qcow2`; the kit's flat file stays untouched.
Delete `vms-system.qcow2` (and `vms-vars.fd`) to start from a fresh kit.

`run-vms.sh` also attaches an empty 8 GB data disk (`vms-data.qcow2`, appears as
`DKA100:` - check with `SHOW DEVICE D`). Keep your own files there so the yearly
kit renewal only replaces the system disk:

    $ INITIALIZE DKA100: USERDATA
    $ MOUNT/SYSTEM DKA100: USERDATA

## Reproduce the flag bug

`test/bt-flags-test.c` needs an x86-64 machine (or `qemu-x86_64` on Linux):

    gcc -O0 -static -o bt test/bt-flags-test.c
    ./bt                                   # real hardware
    qemu-x86_64 -cpu Haswell-noTSX-IBRS ./bt   # QEMU user mode

Look at the `+ btw` lines for "dec from 0" and "dec from -1617": hardware
keeps OF=0 and `jg` not taken; unpatched QEMU sets OF=1 and `jg` taken.

## Notes

- CPU model `Haswell-noTSX-IBRS`. Older models (Nehalem, Opteron_G5, plain
  qemu64) stop early in SYSBOOT; AMD `EPYC` behaves like Haswell.
- `-vga none` keeps OPA0 on the serial line (otherwise the Guest Console takes
  it and output goes to an invisible screen).
- Parallels' x86 emulator cannot run OpenVMS (no XSAVE). UTM uses QEMU and will
  work once the fix is in an upstream QEMU release that UTM ships.
