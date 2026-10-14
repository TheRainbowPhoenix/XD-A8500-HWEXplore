#!/usr/bin/env bash
# Deliberately capture the precise command line; do not hide build failures.
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$ROOT/probe-output"
OUT="$ROOT/probe-output"
SDK="${DEVKITPRO:?DEVKITPRO required}"
LEGACY="$SDK/devkitSH4/bin/sh-elf-gcc"
TOOLCHAIN="${TOOLCHAIN:-devkit}"
CASE="${PROBE_CASE:-legacy}"

if [ "$TOOLCHAIN" = "devkit" ]; then
  CC="$LEGACY"
  FLAGS="-mb -m3 -mexword -mrenesas"
else
  if [ "$TOOLCHAIN" = "hollyhock" ]; then
    CC="$(command -v sh4a_nofpueb-elf-gcc || true)"
  else
    CC="$(command -v sh-elf-gcc || true)"
  fi
  if [ -z "$CC" ]; then echo "COMPILER_MISSING $TOOLCHAIN"; exit 20; fi
  case "$CASE" in
    native|native-link-devkit) FLAGS="" ;;
    m3|m3-link-devkit) FLAGS="-mb -m3" ;;
    m4) FLAGS="-mb -m4-nofpu" ;;
    *) FLAGS="-mb -m3" ;;
  esac
fi

echo "Toolchain=$TOOLCHAIN Case=$CASE CC=$CC FLAGS=$FLAGS"
"$CC" --version | head -n1
"$CC" -Q --help=target 2>&1 | grep -E '(-m3|-m4|-mb|-mrenesas)' | head -15 || true
# Explicitly retain original headers, static libraries and converter, while
# substituting only the C compiler, ISA flags and linker driver.
INC="$SDK/libdataplus/include"
LIB="$SDK/libdataplus/lib"
SOURCE="$ROOT/probes/hello.c"
set -x
"$CC" $FLAGS -O0 -fno-builtin -I"$INC" -c "$SOURCE" -o "$OUT/hello.o" || exit 21
if [ "${LINK_WITH_DEVKIT:-0}" = 1 ]; then
  # Counterfactual experiment: alter ONLY the codegen compiler, retain the
  # working patched devkit link driver, CRT/startup, and EX-word linker specs.
  LINK_CC="$LEGACY"
  LINK_FLAGS="-mb -m3 -mexword -mrenesas"
else
  LINK_CC="$CC"
  LINK_FLAGS="$FLAGS"
fi
"$LINK_CC" $LINK_FLAGS -nostdlib "$OUT/hello.o" -L"$LIB" -ldataplus -lgraphics -lsh4a -Wl,-Map,"$OUT/hello.map" -o "$OUT/hello.elf" || exit 22
"$SDK/tools/bin/elf2d01" -m hlpro "$OUT/hello.elf" "$OUT/hello.d01" || exit 23
set +x
file "$OUT/hello.elf" "$OUT/hello.d01"
# Critical: patched -mexword compiler injects nonstandard startup/linkage.
# Print the ELF evidence rather than treating a produced .d01 as runnable.
for util in "${CC%-gcc}-readelf" "${CC%-gcc}-nm" "${CC%-gcc}-objdump"; do
  command -v "$util" >/dev/null 2>&1 && echo "Found $util" || true
done
if command -v readelf >/dev/null 2>&1; then
  readelf -h "$OUT/hello.elf" > "$OUT/elf-header.txt" || true
  readelf -SW "$OUT/hello.elf" > "$OUT/elf-sections.txt" || true
  readelf -sW "$OUT/hello.elf" > "$OUT/elf-symbols.txt" || true
fi
if command -v objdump >/dev/null 2>&1; then
  objdump -d "$OUT/hello.elf" > "$OUT/elf-disasm.txt" 2>&1 || true
fi
if command -v "${CC%-gcc}-readelf" >/dev/null 2>&1; then
  "${CC%-gcc}-readelf" -hSW "$OUT/hello.elf" > "$OUT/target-elf.txt" || true
fi
"$LINK_CC" -### $LINK_FLAGS -nostdlib "$OUT/hello.o" -L"$LIB" -ldataplus -lgraphics -lsh4a -o "$OUT/inspect.elf" > "$OUT/driver-specs.txt" 2>&1 || true
echo "### ELF entry / defined startup symbols ###"
grep -E 'Entry point' "$OUT/elf-header.txt" 2>/dev/null || true
grep -E "(_start|main)" "$OUT/elf-symbols.txt" 2>/dev/null || true
# Create isolated on-device add-on packages with a unique five-character
# dictionary ID; package both regional metadata sets.
case "$TOOLCHAIN:$CASE" in
  devkit:legacy) APPID=DVHLO ;;
  hollyhock:native-link-devkit) APPID=HCLLO ;;
  hollyhock:native) APPID=HNLL0 ;;
  hollyhock:m3) APPID=H3LLO ;;
  hollyhock:m4) APPID=H4LLO ;;
  gint:m3-link-devkit) APPID=GCLLO ;;
  gint:m3) APPID=G3LLO ;;
  gint:m4) APPID=G4LLO ;;
  *) APPID=PRBLO ;;
esac
# Never package missing-entrypoint builds as if they were deployable.
if [ "$TOOLCHAIN" != devkit ] && [ "${LINK_WITH_DEVKIT:-0}" != 1 ]; then
  echo "WARNING: raw alternate linker can omit _start / EX-word CRT; inspect before device use."
fi
for region in ja cn; do
  PKG="$OUT/package/$region/$APPID"
  mkdir -p "$PKG"
  cp "$OUT/hello.d01" "$PKG/hello.d01"
  for src in "$ROOT"/html/"$region"/*.htm; do
    sed -e "s/@APPTITLE/HELLO $TOOLCHAIN $CASE/g" \\
        -e "s/@APPID/$APPID/g" -e "s/@APPMOD/hello.d01/g" "$src" > "$PKG/$(basename "$src")"
  done
  : > "$PKG/fileinfo.cji"
done
echo "APPID=$APPID"
echo "BUILD_OK $TOOLCHAIN $CASE"
