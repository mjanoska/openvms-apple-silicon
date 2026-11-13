# OpenVMS x86-64 on Apple Silicon

Run **OpenVMS**, the operating system DEC introduced in 1977 with the VAX and
still developed today by VMS Software Inc. (VSI), on an Apple Silicon Mac.
VSI gives out a free, non-commercial community licence with a ready-made
virtual machine. This repo makes that VM run on M-series Macs, using QEMU's
x86 emulation plus a small QEMU fix. Tested on an M4 MacBook Pro with the
2026 community kit (OpenVMS V9.2-3): full boot in about a minute with
4 CPUs / 8 GB.

**New to VMS?** Jump to [Use](#use) and [Your first ten minutes](#your-first-ten-minutes).
The next section explains the bug, for the curious.

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

You need: an Apple Silicon Mac, ~15 GB free disk, Apple's developer tools
(`xcode-select --install`) and [Homebrew](https://brew.sh).

    git clone https://github.com/mjanoska/openvms-apple-silicon.git
    cd openvms-apple-silicon

1. Get the VSI community licence kit (vmssoftware.com, community licence
   program). It arrives as a zip with a VMware disk. Unzip it into this folder:

       unzip ~/Downloads/<your kit>.zip -d .

   The 2026 kit contains `X86_V923-comm-2026.vmdk` (a small descriptor) and
   `X86_V923-comm-2026-flat.vmdk` (the 8 GB disk, raw). No conversion is
   needed: `run-vms.sh` puts a qcow2 overlay on top of the flat file. If a kit
   ships one single `.vmdk` instead, `run-vms.sh` converts it once to
   `vms-base.qcow2`. Do **not** redistribute the kit - it is licensed to you.
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

## Your first ten minutes

When the boot messages stop, press Enter, log in as `SYSTEM` with your kit's
password. The `$` prompt is DCL; commands can be shortened (`DIR` = `DIRECTORY`).

| DCL                         | does                          | Unix      |
|-----------------------------|-------------------------------|-----------|
| `HELP`                      | built-in manual               | `man`     |
| `DIRECTORY`                 | list files                    | `ls -l`   |
| `TYPE LOGIN.COM`            | show a file                   | `cat`     |
| `CREATE/DIRECTORY [.HELLO]` | make a directory              | `mkdir`   |
| `SET DEFAULT [.HELLO]`      | change directory              | `cd`      |
| `EDIT/TPU HELLO.PAS`        | full-screen editor            | `nano`    |
| `PURGE`                     | delete old file versions      | -         |
| `SHOW SYSTEM`               | processes                     | `ps`      |
| `LOGOUT`                    | log out                       | `exit`    |

Every save makes a new file version (`HELLO.PAS;1`, `;2`, ...). A first program:

    $ CREATE HELLO.PAS
    program hello(output);
    begin
      writeln('Hello from OpenVMS on a Mac');
    end.
    (press Ctrl-Z)
    $ PASCAL HELLO
    $ LINK HELLO
    $ RUN HELLO

`SYSTEM` is the administrator (like root); make your own account with
`RUN SYS$SYSTEM:AUTHORIZE`.

## Kickstarter kit: further reading

Inside VMS: `HELP INSTRUCTIONS`, then `HELP <command>`.

- [VSI OpenVMS User's Manual](https://docs.vmssoftware.com/docs/vsi-openvms-user-s-manual.pdf) - the official introduction to VMS and DCL; start here
- [VSI documentation library](https://docs.vmssoftware.com) - all manuals, incl. DCL Dictionary and System Manager's Manual
- [Raymii.org: OpenVMS x86 Getting Started](https://raymii.org/s/blog/OpenVMS_9.2_for_x86_Getting_Started.html) - hobbyist series: licences, network, SSH, software
- [OpenVMS vs. Unix](https://neilrieck.net/docs/vms_vs_unix.html) (Neil Rieck) - for Linux people
- [DECUServe / Eisner](https://eisner.decus.org/) - free account on a long-running public VMS system
- [VSI OpenVMS Forum](https://forum.vmssoftware.com) - VSI engineers and hobbyists
- comp.os.vms - the Usenet newsgroup, via any newsreader
- *Getting Started with OpenVMS: A Guide for New Users*, Michael D. Duffy (Digital Press, 2002)
- [DCL exercises](exercises/README.md) - five practice tasks from an ELTE Budapest course (2005), with links to example solutions
- [VSI community licence](https://vmssoftware.com/community/community-license/) - get your own kit

## Reproduce the flag bug

`test/bt-flags-test.c` needs an x86-64 machine (or `qemu-x86_64` on Linux):

    gcc -O0 -static -o bt test/bt-flags-test.c
    ./bt                                   # real hardware
    qemu-x86_64 -cpu Haswell-noTSX-IBRS ./bt   # QEMU user mode

Look at the `+ btw` lines for "dec from 0" and "dec from -1617": hardware
keeps OF=0 and `jg` not taken; unpatched QEMU sets OF=1 and `jg` taken.

## Verification

Tested on a real Intel Xeon (x86-64 host) with QEMU user mode built from the
same source, stock vs. patched:

| Test | Stock QEMU | Patched QEMU |
|------|-----------|--------------|
| `test/bt-flags-diff.c`: 36,000 cases (BT/BTS/BTR/BTC, 16/32/64-bit, reg/imm/mem, after add/sub/cmp/and/xor/test/dec/inc/neg/shl), flags compared with hardware | 18,734 differ | 0 differ |
| QEMU `make check-tcg` (x86_64-linux-user, 74 tests) | - | all pass |
| QEMU `test-i386` (10,003 result lines) | same output as patched | same output as stock |

Run it yourself on an x86-64 Linux machine:

    gcc -O1 -static -o btdiff test/bt-flags-diff.c
    ./btdiff > hw.txt
    qemu-x86_64 ./btdiff > qemu.txt
    diff hw.txt qemu.txt | grep -c '^<'      # 0 with the patch

Only Intel hardware was checked so far; results from AMD CPUs are welcome.

## Thanks to the people who tried before

This fix stands on two years of other people's attempts. They hit the same
wall and documented it well enough for us to know where to start digging:

- **nix23** and **dmjb** on the VSI forum
  ([Qemu non-KVM installation](https://forum.vmssoftware.com/viewtopic.php?t=8612), April 2023)
  were the first to try pure QEMU emulation, nix23 on old Intel hardware without
  XSAVE and dmjb on an M1 Mac. Both reached `%SYSTEM-W-NOPAGEFILE` and posted
  full configurations and console logs. dmjb retried with QEMU 8.0 and was the
  first to suspect QEMU's x86 emulation itself. That turned out to be right.
- **Volker Halle** joined that thread with boot logs from a working VMware system
  to compare against, and suggested that something was looping on memory
  allocation. That was a sharp first read of the symptom.
- **sparcie** got the boot started in UTM on an M1 Pro, and **thomas@ham** posted
  a detailed UTM log on an M1
  ([Customer experience with hypervisors untested by VSI](https://forum.vmssoftware.com/viewtopic.php?t=8588), 2023).
  thomas@ham noticed that `%SYSINIT-I-CLRERL` never appeared and that RMS
  reported an error. That was the closest anyone got: our trace showed RMS
  failing at exactly that step.
- **floris.fredrikze** (2024, `-cpu max` and UTM), **curlsman** (2024, M2 with
  UTM 4.6.3, crash dumps attached), **redparadox** and **njmontfort**
  ([Working with UTM on Apple Silicon](https://forum.vmssoftware.com/viewtopic.php?f=39&t=9407), 2025)
  kept the question alive with new hardware and new QEMU versions.
- **eschaton** opened [UTM issue #5195](https://github.com/utmapp/UTM/issues/5195)
  (2023) asking for OpenVMS support in UTM, and **tifasoftware** noted there that
  it boots with IDE disks, the controller setup we use too.
- **Karl-KoenigK** started the
  [Parallels forum thread](https://forum.parallels.com/threads/openvms-9-2-for-x86-64.363702/)
  (2024) where the missing XSAVE on Apple Silicon came up.
- **Remy van Elst** ([raymii.org](https://raymii.org/s/blog/OpenVMS_9.2_for_x86_Getting_Started.html)),
  the [Proxmox guide on robertsinfosec](https://robertsinfosec.pages.dev/posts/openvms-on-proxmox)
  and the [wb8tyw KVM write-up](https://wb8tyw-projects.blogspot.com/2025/01/openvms-92-3-community-edition-setup-on.html)
  documented the community kit, the flat-vmdk naming and the `qemu-img` conversion
  we rely on.

If you posted an attempt we missed, open an issue and we'll add you.

## Notes

- CPU model `Haswell-noTSX-IBRS`. Older models (Nehalem, Opteron_G5, plain
  qemu64) stop early in SYSBOOT; AMD `EPYC` behaves like Haswell.
- `-vga none` keeps OPA0 on the serial line (otherwise the Guest Console takes
  it and output goes to an invisible screen).
- Parallels' x86 emulator cannot run OpenVMS (no XSAVE). UTM uses QEMU and will
  work once the fix is in an upstream QEMU release that UTM ships.
