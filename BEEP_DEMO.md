# Legacy BEEP demo

Hold the physical **B** key to enable the legacy hardware buzzer. Hold **N** to send the newer SPU2DSP1 buzzer command. Release either key to send the corresponding stop operation. Back or Power exits, stopping both paths first.

This demo reproduces the factory ROM's legacy clock-output path:

| Register | Physical address | Start write |
| --- | --- | --- |
| `DDCLKR0` | `0xA44C0000` | `0x4801` |
| `DDCLKR1` | `0xA44C0002` | `0x4800` |
| `DDCK_CNTR` | `0xA44C0020` | set bits 0–1 |

`DDCK_CNTR` bits 0–1 are cleared on release/exit.

## DSP/SPU path (N key)

The **N** key reproduces the newer factory-ROM path. It clears the module-stop and clock-gate bits at `MSTPCR2` (`0xA4150038`), `FSICLKCR` (`0xA4150008`), and `SPUCLKCR` (`0xA415003C`), writes the nine-word buzzer descriptor at `0xFE340000`, writes the start command `0x00100009` to the DSP1 command doorbell at `0xFE3FFD38`, then enables DSP1's completion-event bit at `0xFE3FFD2C`. Releasing N writes stop descriptor `0x08200200` and sends command `0x00100001`.

This is a direct hardware test with no runtime model detection. Use B on legacy-clock units and N on DSP-beep units; do not hold both keys at once.

Build with [TheRainbowPhoenix/libdataplus-ex](https://github.com/TheRainbowPhoenix/libdataplus-ex). Its keypad headers provide `KEY_CHAR_B`, used by the demo.
