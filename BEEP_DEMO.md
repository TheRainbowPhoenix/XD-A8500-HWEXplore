# Legacy BEEP demo

Hold the physical **B** key to enable the legacy hardware buzzer. Release it to disable the buzzer. Back or Power exits, with the buzzer disabled first.

This demo reproduces the factory ROM's legacy clock-output path:

| Register | Physical address | Start write |
| --- | --- | --- |
| `DDCLKR0` | `0xA44C0000` | `0x4801` |
| `DDCLKR1` | `0xA44C0002` | `0x4800` |
| `DDCK_CNTR` | `0xA44C0020` | set bits 0–1 |

`DDCK_CNTR` bits 0–1 are cleared on release/exit. The firmware uses an SPU2DSP1-based beep implementation on newer hardware variants; do not use this direct-register demo on those units until a platform check has been added.

Build with [TheRainbowPhoenix/libdataplus-ex](https://github.com/TheRainbowPhoenix/libdataplus-ex). Its keypad headers provide `KEY_CHAR_B`, used by the demo.
