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
    uint32_t addr=(uint32_t)(uintptr_t)test_breakpoint_target;
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
static void render(void)
{
    char h[9];
    set_pen(create_rgb16(255,255,255));draw_rect(0,0,528,320);
    set_pen(create_rgb16(20,44,71));render_text(16,12,"BKPUT / SH4A BREAKPOINT TEST");
    set_pen(create_rgb16(80,101,115));
    render_text(16,40,"LOCAL TARGET ONLY - NO OS PATCH");
    render_text(16,70,"S: ARM UBC  T: CALL TEST  D: DISARM");
    render_text(16,94,"BACK / POWER: EXIT (DISARM FIRST)");
    set_pen(create_rgb16(24,50,74));
    render_text(16,135,armed?"UBC: ARMED":"UBC: DISARMED");
    render_text(16,157,hits?"HIT REGISTER CAPTURED":"NO BREAKPOINT HIT YET");
    hex8(h,(uint32_t)(uintptr_t)test_breakpoint_target);
    render_text(16,184,"TARGET ADDRESS:");render_text(180,184,h);
    if(hits) {
        hex8(h,last_state.r[0]);render_text(16,210,"R0:");render_text(76,210,h);
        hex8(h,last_state.pr);render_text(16,234,"PR:");render_text(76,234,h);
    }
    render_text(16,282,"UBC PRIVILEGED ACCESS MAY REBOOT!");
    lcdc_copy_vram();
}
int main(void)
{
    unsigned old=0u;
    hits=armed=tested=0u;
    render();
    for(;;) {
        unsigned cur=0u, edge;
        volatile unsigned spin;
        keypad_read();
        if(get_key_state(KEY_CHAR_S))cur|=1u;
        if(get_key_state(KEY_CHAR_T))cur|=2u;
        if(get_key_state(KEY_CHAR_D))cur|=4u;
        if(get_key_state(KEY_BACK)||get_key_state(KEY_POWER))cur|=8u;
        edge=cur&~old;old=cur;
        if(edge&8u) {
            if(armed)disarm_breakpoint();
            return -2;
        }
        if(edge&1u)arm_breakpoint();
        if(edge&2u)test_breakpoint_target();
        if(edge&4u) {if(armed)disarm_breakpoint();}
        if(edge)render();
        for(spin=0u;spin<25000u;spin++) { }
    }
}
