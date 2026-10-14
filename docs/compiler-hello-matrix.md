# Compiler compatibility experiment (EX-word DATAPLUS SH4A)

## Goal

This isolates why **Hollyhock GCC Hello previously rebooted on hardware**
while the original **devkitSH4 GCC + precompiled libdataplus** Hello and grid
programs ran correctly.

Five GitHub Actions cases compile **the identical** `probes/hello.c` against
the same **original** libdataplus header and static libraries:

| Case | Container/compiler | ISA flags |
| --- | --- | --- |
| devkit / legacy | Ubuntu + patched `sh-elf-gcc` from MaxSignal | `-mb -m3 -mexword -mrenesas` |
| hollyhock / m3 | `ghcr.io/classpaddev/hollyhock-3:v2.2.2` / `sh4a_nofpueb-elf-gcc` | `-mb -m3` |
| hollyhock / m4 | Same | `-mb -m4-nofpu` |
| gint / m3 | `ghcr.io/therainbowphoenix/gint-docker:main` / `sh-elf-gcc` | `-mb -m3` |
| gint / m4 | Same | `-mb -m4-nofpu` |

All cases use the original `elf2d01` converter from the MaxSignal devkit
archive; this avoids conflating the converter with a GCC difference.

### How to interpret

- **Compile failure:** unsupported ISA flag, missing cross compiler, or header incompatibility.
- **Link failure:** ABI mismatch, missing runtime startup, unresolved symbols, or linker defaults.
- **ELF and D01 produced:** build-only success. **No proof of real-device startup.**
- **Hardware reboot:** compare ELF attributes, disassembly, entry point, section VMA/LMA,
  startup sequence and stack/linkage to the devkit reference.
- **Hardware launch:** confirms the *specific* compiler/flag/library combination on the device.

A toolchain container such as gint is a **compiler distribution** here, not a
portable EX-word runtime. The gint/fxsdk ClassPad OS support should **not**
be linked into these EX-word probes. The goal is to isolate GCC codegen/ABI.

The original `libexword-re` fork is a PC-side USB transport and add-on
installer, **not a substitute** for libdataplus / the EX-word runtime.
After downloading a successful artifact ZIP, extract the `hello.d01`,
place it in a known-good add-on dictionary directory with matching metadata
and use [libexword-re](https://github.com/TheRainbowPhoenix/libexword-re)
to install it. Keep variant IDs distinct; do not overwrite working add-ons.

### CI status

[View experimental Hello toolchain matrix](https://github.com/TheRainbowPhoenix/XD-A8500-HWEXplore/actions/workflows/hello-compiler-matrix.yml).

Artifacts contain raw `hello.elf`, `hello.d01`, linker map, and compiler
diagnostics where possible. The initial matrix is for compilation/linking
diagnosis; the next iteration will package installable variant-specific app
IDs and perform byte-level ELF comparisons for hardware testing.
