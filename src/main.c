#include <graphics/color.h>
#include <graphics/drawing.h>
#include <graphics/init.h>
#include <graphics/lcdc.h>
#include <graphics/sub_lcdc.h>
#include <graphics/text.h>
#include <sh4a/input/accelerometer.h>
#include <sh4a/input/keypad.h>
#include <sh4a/input/touch.h>
#include <stdint.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320

typedef void (*rom_void_function)(void);
typedef void (*rom_int_function)(int);
typedef int (*rom_cursor_function)(uint8_t x, uint8_t y);
typedef int (*rom_text_function)(const char *text, int unknown);

#define ROM_LCD_CLEAR        ((rom_void_function)0x80026354u)
#define ROM_LCD_REFRESH      ((rom_void_function)0x800381E0u)
#define ROM_DEBUG_SET_CURSOR ((rom_cursor_function)0x80026F48u)
#define ROM_DEBUG_PRINT      ((rom_text_function)0x80026F90u)
#define ROM_SUBLCD_PATTERN   ((rom_int_function)0x80077044u)

enum panel_target { PANEL_MAIN, PANEL_SUB };
enum render_mode { MODE_SYSTEM, MODE_BARE };

static void draw_main_paint(uint16_t x, uint16_t y, uint16_t accel_x, uint16_t accel_y, int bare)
{
	struct font *font = get_font();
	if (bare) {
		set_pen(create_rgb16(0, 0, 32));
		draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		set_pen(create_rgb16(255, 255, 255));
		render_text(8, 8, "BARE MAIN LCD: framebuffer + DMAC");
	} else {
		ROM_LCD_CLEAR();
		ROM_DEBUG_SET_CURSOR(1, 0);
		ROM_DEBUG_PRINT("SYSTEM MAIN LCD: ROM clear + refresh", 0);
		set_pen(create_rgb16(255, 255, 255));
	}
	set_pen(create_rgb16(0, 220, 255)); draw_rect(32, 58, 160, 80);
	set_pen(create_rgb16(255, 120, 0)); draw_circle(300, 130, 52);
	set_pen(create_rgb16(100, 255, 100)); draw_line(20, 270, 500, 190);
	set_pen(create_rgb16(255, 40, 220)); draw_rect(x, y, 16, 16);
	set_pen(create_rgb16(255, 255, 0)); draw_rect(accel_x, accel_y, 16, 16);
	set_pen(create_rgb16(255, 255, 255));
	render_text(8, SCREEN_HEIGHT - font->height - 8,
		"B pattern PgUp/Dn size C clear T touch A accel-rest EXE compare");
	if (bare) lcdc_copy_vram(); else ROM_LCD_REFRESH();
}

static void draw_sub_paint(uint16_t x, uint16_t y, uint8_t pattern, uint8_t checker_size, int blank)
{
	uint16_t row, column;
	sub_lcdc_clear(0);
	if (blank) { sub_lcdc_flush(); return; }
	/* Bare equivalents of the factory pattern cases 1..5: solid, 8-pixel
	 * bands, offset bands, border, and crosshair. */
	if (pattern == 1) sub_lcdc_fill_rect(0, 0, SUB_LCDC_WIDTH, SUB_LCDC_HEIGHT, 1);
	else if (pattern == 2 || pattern == 3) {
		for (row = pattern == 2 ? 0 : 4; row < SUB_LCDC_HEIGHT; row += 8)
			sub_lcdc_fill_rect(0, row, SUB_LCDC_WIDTH, 4, 1);
	} else if (pattern == 4) {
		sub_lcdc_fill_rect(0, 0, SUB_LCDC_WIDTH, 2, 1);
		sub_lcdc_fill_rect(0, SUB_LCDC_HEIGHT - 2, SUB_LCDC_WIDTH, 2, 1);
		sub_lcdc_fill_rect(0, 0, 2, SUB_LCDC_HEIGHT, 1);
		sub_lcdc_fill_rect(SUB_LCDC_WIDTH - 2, 0, 2, SUB_LCDC_HEIGHT, 1);
	} else {
		for (row = 0; row < SUB_LCDC_HEIGHT; row += checker_size)
			for (column = 0; column < SUB_LCDC_WIDTH; column += checker_size)
				if (((row / checker_size) ^ (column / checker_size)) & 1u)
					sub_lcdc_fill_rect(column, row, checker_size, checker_size, 1);
	}
	sub_lcdc_fill_rect(x, y, 10, 10, 1);
	sub_lcdc_flush();
}

int main(void)
{
	enum render_mode mode = MODE_BARE;
	uint16_t main_x = 250, main_y = 220, sub_x = 96, sub_y = 42;
	uint8_t sub_pattern = 5;
	uint8_t checker_size = 16;
	int sub_blank = 0, accel_ready = 0, accel_enabled = 0, touch_enabled = 0;
	uint16_t touch_poll_divider = 0;
	uint8_t i2c_poll_divider = 0, accel_poll_divider = 0;
	int accel_rest_x = 0, accel_rest_y = 0;
	int accel_x = 256, accel_y = 152, accel_vx = 0, accel_vy = 0;
	int main_dirty = 1, sub_dirty = 1;
	struct touch_adc_sample touch;
	struct touch_i2c_sample i2c_touch;
	struct accelerometer_sample accelerometer;

	/* Program the R61523 window explicitly: bare rendering must not depend on
	 * the system GUI having configured it before this add-in was entered. */
	graphics_init(SCREEN_WIDTH, SCREEN_HEIGHT, (void *)0xac200000u);
	sub_lcdc_init();
	touch_adc_init();
	while (1) {
		keypad_read();
		if (get_key_state(KEY_POWER) || get_key_state(KEY_BACK)) return -2;
		if (get_key_pressed(KEY_ENTER)) {
			mode = mode == MODE_SYSTEM ? MODE_BARE : MODE_SYSTEM;
			main_dirty = 1;
		}
		if (get_key_pressed(KEY_CHAR_B)) {
			sub_pattern = sub_pattern == 5 ? 1 : sub_pattern + 1;
			sub_blank = 0;
			sub_dirty = 1;
		}
		if (get_key_pressed(KEY_CHAR_C)) {
			sub_blank = 1;
			sub_dirty = 1;
		}
		if (get_key_pressed(KEY_PAGE_UP) && checker_size < 32) { checker_size <<= 1; sub_blank = 0; sub_dirty = 1; }
		if (get_key_pressed(KEY_PAGE_DOWN) && checker_size > 2) { checker_size >>= 1; sub_blank = 0; sub_dirty = 1; }
		if (get_key_pressed(KEY_CHAR_T)) touch_enabled = !touch_enabled;
		if (get_key_pressed(KEY_CHAR_A)) {
			if (!accel_ready) accel_ready = accelerometer_init();
			if (accel_ready && accelerometer_read(&accelerometer)) {
				accel_rest_x = accelerometer.x;
				accel_rest_y = accelerometer.y;
				accel_x = 256; accel_y = 152; accel_vx = 0; accel_vy = 0;
				accel_enabled = 1;
				main_dirty = 1;
			}
		}
		if (get_key_pressed(KEY_LEFT) && main_x >= 8) { main_x -= 8; main_dirty = 1; }
		if (get_key_pressed(KEY_RIGHT) && main_x < SCREEN_WIDTH - 24) { main_x += 8; main_dirty = 1; }
		if (get_key_pressed(KEY_UP) && main_y >= 8) { main_y -= 8; main_dirty = 1; }
		if (get_key_pressed(KEY_DOWN) && main_y < SCREEN_HEIGHT - 24) { main_y += 8; main_dirty = 1; }
		/* Poll the ADC infrequently. A new point performs exactly one sub-LCD
		 * flush; idle spinning never touches either display bus. */
		if (++touch_poll_divider == 0 && touch_adc_read(&touch)) {
			uint16_t x = (uint16_t)((uint32_t)touch.channel_a * (SUB_LCDC_WIDTH - 10u) / 1024u);
			uint16_t y = (uint16_t)((uint32_t)touch.channel_c * (SUB_LCDC_HEIGHT - 10u) / 1024u);
			if (x != sub_x || y != sub_y) { sub_x = x; sub_y = y; sub_dirty = 1; }
		}
		/* Full I2C sample sets are slow (12 two-byte reads). T enables a
		 * periodic probe; rendering remains conditional on a new point. */
		if (touch_enabled && ++i2c_poll_divider == 0 && touch_i2c_read(&i2c_touch)) {
			int tx = ((int)i2c_touch.x - 0x50) * (SCREEN_WIDTH - 16) / (0xff5 - 0x50);
			int ty = ((int)i2c_touch.y - 0x70) * (SCREEN_HEIGHT - 16) / (0xff1 - 0x70);
			if (tx < 0) tx = 0; if (tx > SCREEN_WIDTH - 16) tx = SCREEN_WIDTH - 16;
			if (ty < 0) ty = 0; if (ty > SCREEN_HEIGHT - 16) ty = SCREEN_HEIGHT - 16;
			if (main_x != (uint16_t)tx || main_y != (uint16_t)ty) {
				main_x = (uint16_t)tx; main_y = (uint16_t)ty; main_dirty = 1;
			}
		}
		if (accel_enabled && ++accel_poll_divider == 0 && accelerometer_read(&accelerometer)) {
			accel_vx += ((int)accelerometer.x - accel_rest_x) / 12;
			accel_vy -= ((int)accelerometer.y - accel_rest_y) / 12;
			if (accel_vx > 24) accel_vx = 24; if (accel_vx < -24) accel_vx = -24;
			if (accel_vy > 24) accel_vy = 24; if (accel_vy < -24) accel_vy = -24;
			accel_x += accel_vx; accel_y += accel_vy;
			if (accel_x < 0) { accel_x = 0; accel_vx = -accel_vx; }
			if (accel_x > SCREEN_WIDTH - 16) { accel_x = SCREEN_WIDTH - 16; accel_vx = -accel_vx; }
			if (accel_y < 0) { accel_y = 0; accel_vy = -accel_vy; }
			if (accel_y > SCREEN_HEIGHT - 16) { accel_y = SCREEN_HEIGHT - 16; accel_vy = -accel_vy; }
			main_dirty = 1;
		}
		if (main_dirty) { draw_main_paint(main_x, main_y, (uint16_t)accel_x, (uint16_t)accel_y, mode == MODE_BARE); main_dirty = 0; }
		if (sub_dirty) { draw_sub_paint(sub_x, sub_y, sub_pattern, checker_size, sub_blank); sub_dirty = 0; }
	}
}
