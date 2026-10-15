/* EX-word SH4A UBC breakpoint utility experiment.
 * Adapted from SnailMath/hollyhock-2 demos/breakpoint_util.
 * No direct OS patching, no ROM writes; breakpoint targets only our test function.
 * Privileged UBC/DBR operations still can reboot unsupported hardware.
 */
#include <stdint.h>
#include <graphics/color.h>
#include <graphics/drawing.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>

#define UBC_CBR0 (*(volatile uint32_t *)(uintptr_t)0xFF200000u)
#define UBC_CRR0 (*(volatile uint32_t *)(uintptr_t)0xFF200004u)
#define UBC_CAR0 (*(volatile uint32_t *)(uintptr_t)0xFF200008u)
#define UBC_CAMR0 (*(volatile uint32_t *)(uintptr_t)0xFF20000Cu)
#define UBC_CBCR (*(volatile uint32_t *)(uintptr_t)0xFF200620u)
#define UBC_CBR_ID (1u << 4)
#define UBC_CBR_CE (1u << 0)
#define UBC_CRR_RESERVED (1u << 13)
#define UBC_CRR_PCB (1u << 1)
#define UBC_CRR_BIE (1u << 0)
#define UBC_CBCR_UBDE (1u << 0)

struct CPUState { uint32_t r[16], gbr, pr, mach, macl; };
extern void BreakpointHandlerStub(void);
static volatile unsigned hits;
static volatile unsigned armed;
static volatile unsigned tested;
static volatile unsigned last_test_hits;
static unsigned test_attempted;
static uint32_t breakpoint_address;
static unsigned edit_mode, digit_cursor;
static struct CPUState last_state;
static void (* const stub_address)(void) = BreakpointHandlerStub;
static const char hexdigits[]="0123456789ABCDEF";

void BreakpointHandler(struct CPUState *state)
{
    unsigned i;
    /* Short exception path; avoid graphics calls inside debug exception. */
    UBC_CBR0 = 0u;
    for (i=0u;i<16u;i++) last_state.r[i]=state->r[i];
    last_state.gbr=state->gbr;
    last_state.pr=state->pr;
    last_state.mach=state->mach;
    last_state.macl=state->macl;
    hits++;
    armed=0u;
}

/* Dedicated noinline function with an actual body, designed only to test
 * a breakpoint in this very app. No arbitrary address writes. */
__attribute__((noinline,used)) static void test_breakpoint_target(void)
{
    tested++;
    __asm__ volatile("nop" ::: "memory");
}

static void arm_breakpoint(void)
{
    uint32_t addr=breakpoint_address;
    /* Don't arm an odd address or unaligned SH instruction fetch. */
    if (addr & 1u) return;
    /* Writes require privileged mode and may trap on EX-word OS. */
    __asm__ volatile("mov %0,r0\n\t.word 0x40fa" : : "r"(stub_address) : "r0","memory");
    UBC_CBCR=UBC_CBCR_UBDE;
    UBC_CBR0=UBC_CBR_ID;
    UBC_CAR0=addr;
    UBC_CAMR0=0u;
    UBC_CRR0=UBC_CRR_RESERVED|UBC_CRR_PCB|UBC_CRR_BIE;
    UBC_CBR0=UBC_CBR_ID|UBC_CBR_CE;
    armed=1u;
}
static void disarm_breakpoint(void)
{
    UBC_CBR0=0u;
    armed=0u;
}
static void hex8(char *o,uint32_t v)
{
    unsigned i;
    for(i=0;i<8;i++)o[i]=hexdigits[(v>>(28u-4u*i))&15u];
    o[8]=0;
}
static const unsigned keys[] = {
    KEY_CHAR_S,KEY_CHAR_T,KEY_CHAR_D,KEY_CHAR_J,
    KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,KEY_ENTER,KEY_BACKSPACE,
    KEY_BACK,KEY_POWER,
    KEY_CHAR_Q,KEY_CHAR_W,KEY_CHAR_E,KEY_CHAR_R,KEY_CHAR_T,
    KEY_CHAR_Y,KEY_CHAR_U,KEY_CHAR_I,KEY_CHAR_O,KEY_CHAR_P,
    KEY_CHAR_A,KEY_CHAR_B,KEY_CHAR_C,KEY_CHAR_D,KEY_CHAR_E,KEY_CHAR_F
};
static uint32_t snapshot(void)
{
    unsigned i;
    uint32_t bits=0u;
    for(i=0u;i<sizeof(keys)/sizeof(keys[0]);i++)
        if(get_key_state((uint8_t)keys[i])) bits|=1u<<i;
    return bits;
}
#define BIT(n) (1u << (n))
static void nibble_set(unsigned digit)
{
    unsigned shift=(7u-digit_cursor)*4u;
    breakpoint_address=(breakpoint_address&~(15u<<shift))|
                       ((digit&15u)<<shift);
    if(digit_cursor<7u)digit_cursor++;
}
static void nibble_delta(unsigned inc)
{
    unsigned shift=(7u-digit_cursor)*4u;
    unsigned value=(breakpoint_address>>shift)&15u;
    unsigned old=digit_cursor;
    nibble_set((value+(inc?1u:15u))&15u);
    digit_cursor=old;
}
static void render(void)
{
    char h[9];
    set_pen(create_rgb16(255,255,255));draw_rect(0,0,528,320);
    set_pen(create_rgb16(20,44,71));render_text(16,12,"BKPUT / IN-APP UBC DIAGNOSTIC");
    set_pen(create_rgb16(80,101,115));
    render_text(16,40,"J: SET ADDRESS  S: ARM  D: DISARM");
    render_text(16,70,edit_mode?"Q-P:0-9 A-F:HEX  LR:DIGIT":"T: LOCAL SELF-TEST (USE DEFAULT ADDRESS)");
    render_text(16,94,edit_mode?"UP/DN:INCREMENT  ENTER:OK BACK:CANCEL":"BREAKPOINTS DO NOT SURVIVE APP EXIT");
    set_pen(create_rgb16(24,50,74));
    render_text(16,135,armed?"UBC: ARMED":"UBC: DISARMED");
    render_text(16,157,hits?"HIT REGISTER CAPTURED":"NO BREAKPOINT HIT YET");
    if(test_attempted) render_text(16,257,last_test_hits ? "SELF-TEST: HIT RECORDED":"SELF-TEST: NO HIT RECORDED");
    hex8(h,breakpoint_address);
    render_text(16,184,"TARGET ADDRESS:");render_text(180,184,h);
    if(edit_mode) render_text(16,201,"EDITING 8 HEX DIGITS - POSITION BELOW");
    if(hits) {
        hex8(h,last_state.r[0]);render_text(16,210,"R0:");render_text(76,210,h);
        hex8(h,last_state.pr);render_text(16,234,"PR:");render_text(76,234,h);
    }
    render_text(16,282,"UBC MMIO MAY REBOOT; NOT PERSISTENT");
    lcdc_copy_vram();
}
int main(void)
{
    uint32_t old=0u, saved=0u;
    hits=armed=tested=0u;
    test_attempted=0u;last_test_hits=0u;
    breakpoint_address=(uint32_t)(uintptr_t)test_breakpoint_target;
    edit_mode=0u;digit_cursor=0u;
    render();
    for(;;) {
        uint32_t current,pressed;
        unsigned i;
        volatile unsigned spin;
        keypad_read();
        current=snapshot();
        pressed=current&~old;old=current;
        if(edit_mode) {
            if(pressed&(BIT(10)|BIT(11))) {
                breakpoint_address=saved;
                edit_mode=0u;
            } else if(pressed&BIT(8)) {
                edit_mode=0u;
            } else if(pressed&BIT(9)) {
                nibble_set(0u);
            } else if(pressed&BIT(4)) {
                if(digit_cursor>0u)digit_cursor--;
            } else if(pressed&BIT(5)) {
                if(digit_cursor<7u)digit_cursor++;
            } else if(pressed&BIT(6)) {
                nibble_delta(1u);
            } else if(pressed&BIT(7)) {
                nibble_delta(0u);
            } else {
                for(i=0u;i<10u;i++) {
                    if(pressed&BIT(12u+i)) {nibble_set(i);break;}
                }
                if(i==10u) {
                    for(i=0u;i<6u;i++) {
                        if(pressed&BIT(22u+i)) {nibble_set(i+10u);break;}
                    }
                }
            }
        } else {
            if(pressed&(BIT(10)|BIT(11))) {
                if(armed)disarm_breakpoint();
                return -2;
            }
            if((pressed&BIT(3))&&!armed) {
                saved=breakpoint_address;
                edit_mode=1u;
                digit_cursor=0u;
            }
            if((pressed&BIT(0))&&!armed&&!(breakpoint_address&1u))
                arm_breakpoint();
            if(pressed&BIT(1)) {
                unsigned before=hits;
                test_breakpoint_target();
                last_test_hits=(hits!=before);
                test_attempted=1u;
            }
            if((pressed&BIT(2))&&armed)disarm_breakpoint();
        }
        if(pressed)render();
        for(spin=0u;spin<25000u;spin++) { }
    }
}
