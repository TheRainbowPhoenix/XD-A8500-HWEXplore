# BKPUT — minimal SH4A breakpoint utility (EX-word devkitSH4)

This is a **hardware-experimental** port of [SnailMath's Hollyhock 2
Breakpoint Utility](https://github.com/SnailMath/hollyhock-2/tree/master/demos/breakpoint_util)
to the original **devkitSH4 + original prebuilt libdataplus** used by
EXplorer and previously working EX-word tools.

It is not a Hollyhock app and doesn't use Hollyhock GUI or the ClassPad
runtime. Its white-screen UI uses familiar EX-word `graphics/*` and
`sh4a/input/keypad.h` APIs.

## Why a breakpoint does not fire in the EX-word menu

**BKPUT is an ordinary add-on, not a resident OS debugger.** Pressing
Back or Power exits its main function and normally unloads its code/data.
The program also explicitly calls `disarm_breakpoint()` on exit when
armed. Therefore a breakpoint configured in BKPUT will **not trigger
later in the main OS menu**. Leaving a DBR vector pointing into unloaded
BKPUT code would be unsafe and could reboot or corrupt the running OS.

Before debugging OS/menu code, establish whether UBC works at all in
this add-on: keep the default local test function address, press **S**
to arm, then **T** while BKPUT remains on screen. The latest UI will
show `SELF-TEST: HIT RECORDED` if the exception callback increments its
counter, otherwise `SELF-TEST: NO HIT RECORDED`. The latter does not
unambiguously distinguish inaccessible UBC registers, missing exception
routing, and mismatched instruction addresses.

Supporting breakpoints across OS menu transitions requires a **separate
resident handler with an OS-supported lifetime and exception integration**,
not merely removing BKPUT's disarm-on-exit line. No such resident handler
has been demonstrated safe on this EX-word model.

## Operation

- Starts with **no breakpoint armed**. It draws a plain UI and waits.
- **J** opens the 8-digit hexadecimal breakpoint address editor, initially
  filled with the address of the built-in local test function.
  Q/W/E/R/T/Y/U/I/O/P enter digits 0–9, A–F enter hex digits A–F,
  Left/Right move between nibbles, Up/Down increase/decrease the nibble,
  Backspace clears a nibble, Enter accepts, Back cancels.
- **S** arms UBC channel 0 at the selected *even* instruction address
  (which may be within an app or OS code; this is not a safety check).
- **T** executes that local function. If the debug exception works, the
  handler disables channel 0, saves a minimal register snapshot, and
  returns to the UI; **R0** and **PR** are displayed.
- **D** disarms channel 0.
- **Back / Power** disarms if this app considers it armed, then returns.

The source includes a small SH assembler exception stub based on the
Hollyhock sample's save/restore logic. Unlike Hollyhock's default debug
text API, the exception path does **not call graphics functions**;
it only copies registers and increments a hit counter. UI redrawing
happens back in the ordinary input loop.

## Critical hardware cautions

- **UBC registers at 0xFF200000 etc., and DBR require privileged access.**
  The stock EX-word firmware might reject these operations, instantly
  crash/reboot, or route exceptions differently. We have not verified this
  on the XD-A8500, so **S is dangerous**.
- The local test function is the *default* target, but arbitrary even
  addresses can now be selected with J. An aligned address is **not**
  necessarily executable or mapped. Installing a breakpoint in ROM or OS
  code is much riskier than the local self-test.
- **This implementation replaces DBR when armed and does not restore the
  old DBR value after disarming.** Do not arm it on a device where another
  debugger/debug infrastructure is installed or expected. It must never
  be described as harmless to OS debugging state.
- A hardware-triggered exception can occur with interrupt state, stack,
  and register banking different from Hollyhock's ClassPad expectations.
  The register handler is experimental; crash/reboot is possible.
- Software deliberately writes UBC MMIO configuration registers. It does
  not flash ROM or patch executable memory.

### Suggested test order

1. Install `BKPUT` alongside the existing known-working utilities.
2. Launch it **without pressing S**. Verify the plain screen stays open
   and Back returns normally. If even startup fails, stop: breakpoint
   hardware cannot be the cause.
3. Only if willing to risk an immediate reboot, launch it again, keep
   the default local target, press **S** to arm the test and then **T**. Record whether the screen shows
   a capture, hangs, or reboots.
4. Open the **J** editor to choose an OS/app address *only after*
   the local test has been demonstrated working safely.

The GitHub Actions workflow builds JA/CN installable `BKPUT` packages.
Passing the CI build **does not validate privileged UBC access or the
exception handler on a physical EX-word**.

## Installation

Download `breakpoint-bkput-ja` from the
[breakpoint workflow](https://github.com/TheRainbowPhoenix/XD-A8500-HWEXplore/actions/workflows/breakpoint-devkit.yml),
extract the `BKPUT` directory, and install with libexword-re using
your existing authentication: `setpath mem:///` then `dict install BKPUT`.
Do **not** use `dict reset`.

Related reference: [EXplorer](https://github.com/brain-hackers/EXplorer).
