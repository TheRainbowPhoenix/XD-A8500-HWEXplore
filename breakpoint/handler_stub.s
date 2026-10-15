/* Original register capture concept: SnailMath hollyhock-2 breakpoint_util.
 * Ported to the patched devkitSH4 assembler and C entry point.
 * Only use on hardware where UBC debug exception routing is supported. */
.global _BreakpointHandlerStub
.type _BreakpointHandlerStub,@function
_BreakpointHandlerStub:
    sts.l pr,@-r15
    mov.l r0,@-r15
    mov.l r1,@-r15
    mov.l r2,@-r15
    mov.l r3,@-r15
    mov.l r4,@-r15
    mov.l r5,@-r15
    mov.l r6,@-r15
    mov.l r7,@-r15
    mov.l r8,@-r15
    mov.l r9,@-r15
    mov.l r10,@-r15
    mov.l r11,@-r15
    mov.l r12,@-r15
    mov.l r13,@-r15
    mov.l r14,@-r15
    mov.l _state_ptr,r4
    stc r0_bank,r5
    mov.l r5,@(0,r4)
    stc r1_bank,r5
    mov.l r5,@(4,r4)
    stc r2_bank,r5
    mov.l r5,@(8,r4)
    stc r3_bank,r5
    mov.l r5,@(12,r4)
    stc r4_bank,r5
    mov.l r5,@(16,r4)
    stc r5_bank,r5
    mov.l r5,@(20,r4)
    stc r6_bank,r5
    mov.l r5,@(24,r4)
    stc r7_bank,r5
    mov.l r5,@(28,r4)
    mov.l r8,@(32,r4)
    mov.l r9,@(36,r4)
    mov.l r10,@(40,r4)
    mov.l r11,@(44,r4)
    mov.l r12,@(48,r4)
    mov.l r13,@(52,r4)
    mov.l r14,@(56,r4)
    mov r15,r6
    mov #64,r7
    add r7,r6
    mov.l r6,@(60,r4)
    mov r4,r7
    add #64,r7
    stc gbr,r6
    mov.l r6,@r7
    add #4,r7
    sts pr,r6
    mov.l r6,@r7
    add #4,r7
    sts mach,r6
    mov.l r6,@r7
    add #4,r7
    sts macl,r6
    mov.l r6,@r7
    mov.l _handler_addr,r0
    jsr @r0
    nop
    mov.l @r15+,r14
    mov.l @r15+,r13
    mov.l @r15+,r12
    mov.l @r15+,r11
    mov.l @r15+,r10
    mov.l @r15+,r9
    mov.l @r15+,r8
    mov.l @r15+,r7
    mov.l @r15+,r6
    mov.l @r15+,r5
    mov.l @r15+,r4
    mov.l @r15+,r3
    mov.l @r15+,r2
    mov.l @r15+,r1
    mov.l @r15+,r0
    lds.l @r15+,pr
    stc sgr,r15
    rte
    nop
.align 2
_handler_addr:
.long _BreakpointHandler
_state_ptr:
.long _saved_state
.section .bss
.balign 4
_saved_state:
.space 80
