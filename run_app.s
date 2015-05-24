
.equ APP_ADDR,          0x08020000
.equ SCB_VTOR,          0xE000ED08

.thumb
.thumb_func
.syntax unified
.section .text

.global run_app
run_app:
    ldr     r0, =APP_ADDR
    ldr     r1, =SCB_VTOR
    str     r0, [r1]        @ relocate vector table
    dsb
    ldr     sp, [r0, #0]    @ initialize stack pointer
    ldr     r0, [r0, #4]    @ load reset address
    bx      r0              @ jump to application