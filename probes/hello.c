#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>
/* Intentionally no libc and no initialized globals. The original prebuilt
 * libdataplus handles runtime/graphics; all experiments use identical C. */
int main(void)
{
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 0, 528, 320);
    set_pen(create_rgb16(255, 255, 255));
    render_text(10, 24, "EXWORD HELLO - TOOLCHAIN PROBE");
    lcdc_copy_vram();
    for (;;) {
        keypad_read();
        if (get_key_state(KEY_BACK) || get_key_state(KEY_POWER))
            return -2;
    }
}
