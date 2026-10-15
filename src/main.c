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

static void legacy_beep_start(void) {
    CMOD_DDCLKR0 = LEGACY_BEEP_CLOCK | 1u;
    CMOD_DDCLKR1 = LEGACY_BEEP_CLOCK;
    CMOD_DDCK_CNTR |= 3u;
}

static void legacy_beep_stop(void) {
    CMOD_DDCK_CNTR &= (uint8_t)~3u;
}

int main(void) {
    struct font *fnt = get_font();
    int beeping = 0;

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
        (SCREEN_WIDTH - (sizeof "Back/Power: exit" - 1) * fnt->width) / 2,
        SCREEN_HEIGHT / 2 + fnt->height,
        "Back/Power: exit"
    );

    lcdc_copy_vram();

    while (1) {
        keypad_read();

        if (get_key_state(KEY_CHAR_B)) {
            if (!beeping)
                legacy_beep_start();
            beeping = 1;
        } else if (beeping) {
            legacy_beep_stop();
            beeping = 0;
        }

        if (get_key_state(KEY_POWER) || get_key_state(KEY_BACK)) {
            legacy_beep_stop();
            return -2;
        }
    }
}
