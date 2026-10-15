# LCD / sub-LCD comparison add-in

This add-in compares ROM32A system calls with bare, reverse-engineered drivers.

- **F1** toggles system and bare mode.
- **F2** selects the main colour LCD or the 208x64 monochrome sub-LCD.
- In system sub-LCD mode, **EXE** cycles factory-ROM test patterns.
- In bare sub-LCD mode, the add-in fills and flushes its own 26-byte-by-64-row shadow framebuffer.
- **T** reads the raw ADC touch path and places the bare sub-LCD marker using uncalibrated values.

The system main-LCD path uses ROM `LCD_ClearScreen` and `LCD_RefreshDisplay`; the bare path uses libdataplus-ex's framebuffer and DMAC transfer. The bare sub-LCD path is independent of all ROM sub-LCD routines.
