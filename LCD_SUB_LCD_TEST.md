# LCD / sub-LCD comparison add-in

This add-in compares ROM32A system calls with bare, reverse-engineered drivers.

- The add-in **boots in bare mode** and continuously refreshes both panels.
- **F1** toggles the ROM/system comparison mode.
- **F2** selects the main colour LCD or the 208x64 monochrome sub-LCD.
- In system sub-LCD mode, **EXE** cycles factory-ROM test patterns.
- In bare sub-LCD mode, the add-in fills and flushes its own 26-byte-by-64-row shadow framebuffer.
- The raw ADC touch path continuously moves the bare sub-LCD marker using uncalibrated values.
- **F4** probes the independent I2C touch controller once. A valid sample moves the main-LCD marker. This bounded, experimental polling driver is reconstructed from the ROM and the SH7724 I2C register model; a failure leaves the marker unchanged.

The system main-LCD path uses ROM `LCD_ClearScreen` and `LCD_RefreshDisplay`; the bare path explicitly programs the R61523 window then uses libdataplus-ex's framebuffer and DMAC transfer. The bare sub-LCD path resets PNDR.3 and sends the reconstructed controller initialization sequence before using its own framebuffer; it is independent of all ROM sub-LCD routines.
