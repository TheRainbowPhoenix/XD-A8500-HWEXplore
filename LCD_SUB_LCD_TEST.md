# LCD / sub-LCD comparison add-in

This add-in compares ROM32A system calls with bare, reverse-engineered drivers.

- The add-in **boots in bare mode**. It only transfers a display when content changes; it never continuously DMA-refreshes while idle.
- **EXE** toggles the ROM/system main-LCD comparison view.
- **B** cycles five bare sub-LCD patterns adapted from the factory display-pattern test: solid, aligned bands, offset bands, border, and crosshair.
- The fifth pattern is a full-panel checkerboard. **Page Up/Page Down** double/halve its square size. **C** clears the sub-LCD completely; B or Page Up/Page Down redraws it.
- **T** toggles periodic, bounded I2C touch sampling. A new raw point moves the magenta main-LCD marker, using the ROM's accepted sample range as its initial scaling. The driver follows the ROM's clock-divider selection, discard-first conversion, five-sample filter, and pen-down gate.
- **A** initializes the directly reconstructed accelerometer I2C device and records its current position as rest. Tilting then accelerates the yellow square, which bounces at the display edges; arrow keys move the magenta marker manually.
- The raw ADC path is polled at a low rate and updates the bare sub-LCD marker only when its uncalibrated coordinates change.

The system main-LCD path uses ROM `LCD_ClearScreen` and `LCD_RefreshDisplay`; the bare path explicitly programs the R61523 window then uses libdataplus-ex's framebuffer and DMAC transfer. The bare sub-LCD path resets PNDR.3 and sends the reconstructed controller initialization sequence before using its own framebuffer; it is independent of all ROM sub-LCD routines.
