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
 * Newer-unit DSP beep path. The DSP cores require their ROM firmware load,
 * interrupt-handler installation and I2C audio-codec setup before accepting
 * the buzzer descriptor. Use the factory-ROM entry points for that complete
 * initialization/command sequence rather than trying to partially recreate
 * it in the addon.
 */
typedef void (*rom_void_function)(void);

#define ROM_SPU2DSP_INIT       ((rom_void_function)0x80061334u)
#define ROM_SPU2DSP1_START_BEEP ((rom_void_function)0x80061988u)
#define ROM_SPU2DSP1_STOP_BEEP  ((rom_void_function)0x80061A4Eu)

static void legacy_beep_start(void) {
    CMOD_DDCLKR0 = LEGACY_BEEP_CLOCK | 1u;
    CMOD_DDCLKR1 = LEGACY_BEEP_CLOCK;
    CMOD_DDCK_CNTR |= 3u;
}

static void legacy_beep_stop(void) {
    CMOD_DDCK_CNTR &= (uint8_t)~3u;
}

static void dsp_beep_start(void) {
    ROM_SPU2DSP1_START_BEEP();
}

static void dsp_beep_stop(void) {
    ROM_SPU2DSP1_STOP_BEEP();
}

int main(void) {
    struct font *fnt = get_font();
    int legacy_beeping = 0;
    int dsp_beeping = 0;
    int dsp_initialized = 0;

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
            if (!dsp_beeping) {
                if (!dsp_initialized) {
                    ROM_SPU2DSP_INIT();
                    dsp_initialized = 1;
                }
                dsp_beep_start();
            }
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
