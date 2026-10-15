#include <graphics/color.h>
#include <graphics/drawing.h>
#include <graphics/init.h>
#include <graphics/lcdc.h>
#include <graphics/sub_lcdc.h>
#include <graphics/text.h>
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

static void draw_main_paint(uint16_t x, uint16_t y, int bare)
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
	set_pen(create_rgb16(255, 255, 255));
	render_text(8, SCREEN_HEIGHT - font->height - 8,
		"Bare boot: ADC=sub  F4=probe I2C  F1=ROM compare");
	if (bare) lcdc_copy_vram(); else ROM_LCD_REFRESH();
}

static void draw_sub_paint(uint16_t x, uint16_t y)
{
	sub_lcdc_clear(0);
	sub_lcdc_fill_rect(4, 4, 96, 14, 1);
	sub_lcdc_fill_rect(108, 4, 96, 14, 1);
	sub_lcdc_fill_rect(18, 28, 172, 4, 1);
	sub_lcdc_fill_rect(x, y, 10, 10, 1);
	sub_lcdc_flush();
}

int main(void)
{
	enum panel_target panel = PANEL_MAIN;
	enum render_mode mode = MODE_BARE;
	uint16_t main_x = 250, main_y = 220, sub_x = 96, sub_y = 42;
	int sub_pattern = 4;
	struct touch_adc_sample touch;
	struct touch_i2c_sample i2c_touch;

	/* Program the R61523 window explicitly: bare rendering must not depend on
	 * the system GUI having configured it before this add-in was entered. */
	graphics_init(SCREEN_WIDTH, SCREEN_HEIGHT, (void *)0xac200000u);
	sub_lcdc_init();
	touch_adc_init();
	draw_main_paint(main_x, main_y, 1);
	draw_sub_paint(sub_x, sub_y);
	while (1) {
		keypad_read();
		if (get_key_state(KEY_POWER) || get_key_state(KEY_BACK)) return -2;
		/* ENTER is deliberately the primary toggle; F1 is a secondary shortcut. */
		if (get_key_pressed(KEY_ENTER) || get_key_pressed(KEY_FUNC_1)) mode = mode == MODE_SYSTEM ? MODE_BARE : MODE_SYSTEM;
		if (get_key_pressed(KEY_FUNC_2)) panel = panel == PANEL_MAIN ? PANEL_SUB : PANEL_MAIN;
		if (get_key_pressed(KEY_FUNC_3)) { sub_pattern = 4; main_x = 250; main_y = 220; sub_x = 96; sub_y = 42; }
		/* F4 uses the independently reconstructed I2C path once, avoiding
		 * continuous bus traffic while its physical panel mapping is verified. */
		if (get_key_pressed(KEY_FUNC_4) && touch_i2c_read(&i2c_touch)) {
			main_x = (uint16_t)((uint32_t)i2c_touch.x * (SCREEN_WIDTH - 16u) / 4096u);
			main_y = (uint16_t)((uint32_t)i2c_touch.y * (SCREEN_HEIGHT - 16u) / 4096u);
		}
		if (get_key_pressed(KEY_LEFT)) { if (panel == PANEL_SUB && sub_x >= 4) sub_x -= 4; if (panel == PANEL_MAIN && main_x >= 8) main_x -= 8; }
		if (get_key_pressed(KEY_RIGHT)) { if (panel == PANEL_SUB && sub_x < SUB_LCDC_WIDTH - 14) sub_x += 4; if (panel == PANEL_MAIN && main_x < SCREEN_WIDTH - 24) main_x += 8; }
		if (get_key_pressed(KEY_UP)) { if (panel == PANEL_SUB && sub_y >= 4) sub_y -= 4; if (panel == PANEL_MAIN && main_y >= 8) main_y -= 8; }
		if (get_key_pressed(KEY_DOWN)) { if (panel == PANEL_SUB && sub_y < SUB_LCDC_HEIGHT - 14) sub_y += 4; if (panel == PANEL_MAIN && main_y < SCREEN_HEIGHT - 24) main_y += 8; }
		/* Raw ADC touch drives the bare-panel marker whenever its electrode pairs validate. */
		if (touch_adc_read(&touch)) {
			sub_x = (uint16_t)((uint32_t)touch.channel_a * (SUB_LCDC_WIDTH - 10u) / 1024u);
			sub_y = (uint16_t)((uint32_t)touch.channel_c * (SUB_LCDC_HEIGHT - 10u) / 1024u);
		}
		if (mode == MODE_BARE) {
			draw_main_paint(main_x, main_y, 1);
			draw_sub_paint(sub_x, sub_y);
		} else if (panel == PANEL_MAIN) {
			draw_main_paint(main_x, main_y, 0);
		} else {
			if (get_key_pressed(KEY_ENTER)) sub_pattern = sub_pattern == 7 ? 1 : sub_pattern + 1;
			ROM_SUBLCD_PATTERN(sub_pattern);
		}
	}
}
