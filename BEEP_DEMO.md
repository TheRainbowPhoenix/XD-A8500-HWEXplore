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

The **N** key now invokes the complete newer factory-ROM path. On first use it calls the ROM's SPU2/DSP initializer (`0x80061334`), which releases `MSTPCR2`, FSI and SPU clock gates; resets both DSP cores; DMA-loads their firmware into the DSP memories; installs the DSP completion handlers; and configures the audio codec over I²C. It then calls the ROM's DSP1 beep start (`0x80061988`) and stop (`0x80061A4E`) routines. Those routines write the descriptor at `0xFE340000`, use the DSP1 command doorbell at `0xFE3FFD38`, and process completion events through the ROM-installed handlers.

This is a direct hardware test with no runtime model detection. Use B on legacy-clock units and N on DSP-beep units; do not hold both keys at once.

Build with [TheRainbowPhoenix/libdataplus-ex](https://github.com/TheRainbowPhoenix/libdataplus-ex). Its keypad headers provide `KEY_CHAR_B`, used by the demo.
