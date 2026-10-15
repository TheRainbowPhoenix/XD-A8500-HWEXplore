#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>
#include <stdint.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320

/*
 * Legacy EX-word buzzer / dedicated-clock registers, verified against the
 * factory-ROM path TEST_OTHERS_MENU -> BEEP:
 *
 *   DDCLKR0 = 0x4801; DDCLKR1 = 0x4800; DDCK_CNTR |= 3;
 *
 * This is intentionally the legacy clock-output path only. Newer units use
 * an SPU2DSP1 descriptor and mailbox command instead.
 */
#define CMOD_DDCLKR0   (*(volatile uint16_t *)0xA44C0000u)
#define CMOD_DDCLKR1   (*(volatile uint16_t *)0xA44C0002u)
#define CMOD_DDCK_CNTR (*(volatile uint8_t  *)0xA44C0020u)

#define LEGACY_BEEP_CLOCK 0x4800u

/*
 * Newer-unit DSP beep path, copied from FUN_80061988 / FUN_80061a4e in the
 * factory ROM.  The descriptor is host-visible DSP1 command RAM; COM0 is the
 * DSP1 command doorbell and IESETD requests the completion event.
 */
#define CLOCK_FSICLKCR  (*(volatile uint32_t *)0xA4150008u)
#define CLOCK_SPUCLKCR  (*(volatile uint32_t *)0xA415003Cu)
#define POWER_MSTPCR2   (*(volatile uint32_t *)0xA4150038u)
#define IO_PCDR         (*(volatile uint8_t  *)0xA4050124u)
#define IO_PUDR         (*(volatile uint8_t  *)0xA4050162u)

#define SPU2DSP1_DESC   ((volatile uint32_t *)0xFE340000u)
#define SPU2DSP1_IESETD (*(volatile uint32_t *)0xFE3FFD2Cu)
#define SPU2DSP1_COM0   (*(volatile uint32_t *)0xFE3FFD38u)

#define SPU2DSP_EVENT_ENABLE 0x00010000u
#define DSP_BEEP_START_CMD    0x00100009u
#define DSP_BEEP_STOP_CMD     0x00100001u
#define DSP_BEEP_LEVEL         0x27u

static void legacy_beep_start(void) {
    CMOD_DDCLKR0 = LEGACY_BEEP_CLOCK | 1u;
    CMOD_DDCLKR1 = LEGACY_BEEP_CLOCK;
    CMOD_DDCK_CNTR |= 3u;
}

static void legacy_beep_stop(void) {
    CMOD_DDCK_CNTR &= (uint8_t)~3u;
}

static void dsp_beep_start(void) {
    /* FUN_800613f2: release FSI/SPU clocks and SPU power-module stop. */
    CLOCK_FSICLKCR &= ~0x100u;
    CLOCK_SPUCLKCR &= ~0x100u;
    POWER_MSTPCR2 &= ~0x200u;

    /* FUN_80034fb8 enables the output-related GPIO state. */
    IO_PCDR |= 2u;
    IO_PUDR |= 1u;

    /* FUN_80061988's 9-word DSP1 buzzer descriptor. */
    SPU2DSP1_DESC[0] = 0x08200000u;
    SPU2DSP1_DESC[1] = 0u;
    SPU2DSP1_DESC[2] = 0u;
    SPU2DSP1_DESC[3] = 0x00000A00u;
    SPU2DSP1_DESC[4] = 0u;
    SPU2DSP1_DESC[5] = 0x00000300u;
    SPU2DSP1_DESC[6] = DSP_BEEP_LEVEL << 8;
    SPU2DSP1_DESC[7] = 0x0000A000u;
    SPU2DSP1_DESC[8] = 0x00000100u;

    SPU2DSP1_COM0 = DSP_BEEP_START_CMD;
    SPU2DSP1_IESETD |= SPU2DSP_EVENT_ENABLE;
}

static void dsp_beep_stop(void) {
    /* FUN_80061a4e: DSP buzzer-stop descriptor followed by command 1. */
    SPU2DSP1_DESC[0] = 0x08200200u;
    SPU2DSP1_COM0 = DSP_BEEP_STOP_CMD;
    SPU2DSP1_IESETD |= SPU2DSP_EVENT_ENABLE;
}

int main(void) {
    struct font *fnt = get_font();
    int legacy_beeping = 0;
    int dsp_beeping = 0;

    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    set_pen(create_rgb16(255, 255, 255));

    render_text(
        (SCREEN_WIDTH - (sizeof "Legacy BEEP demo" - 1) * fnt->width) / 2,
        SCREEN_HEIGHT / 2 - fnt->height,
        "Legacy BEEP demo"
    );

    render_text(
        (SCREEN_WIDTH - (sizeof "Hold B: beep" - 1) * fnt->width) / 2,
        SCREEN_HEIGHT / 2,
        "Hold B: beep"
    );

    render_text(
        (SCREEN_WIDTH - (sizeof "Hold N: DSP beep" - 1) * fnt->width) / 2,
        SCREEN_HEIGHT / 2 + fnt->height,
        "Hold N: DSP beep"
    );

    render_text(
        (SCREEN_WIDTH - (sizeof "Back/Power: exit" - 1) * fnt->width) / 2,
        SCREEN_HEIGHT / 2 + 2 * fnt->height,
        "Back/Power: exit"
    );

    lcdc_copy_vram();

    while (1) {
        keypad_read();

        if (get_key_state(KEY_CHAR_B)) {
            if (!legacy_beeping)
                legacy_beep_start();
            legacy_beeping = 1;
        } else if (legacy_beeping) {
            legacy_beep_stop();
            legacy_beeping = 0;
        }

        if (get_key_state(KEY_CHAR_N)) {
            if (!dsp_beeping)
                dsp_beep_start();
            dsp_beeping = 1;
        } else if (dsp_beeping) {
            dsp_beep_stop();
            dsp_beeping = 0;
        }

        if (get_key_state(KEY_POWER) || get_key_state(KEY_BACK)) {
            if (legacy_beeping)
                legacy_beep_stop();
            if (dsp_beeping)
                dsp_beep_stop();
            return -2;
        }
    }
}
