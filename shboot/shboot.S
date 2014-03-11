#include "shboot.h"
	b 	reset @ 0x0
    b   undef   @ 0x4 UND
    b swi
    b pref
    b data
    b . // reserved
    b irq
    b firq

.macro addruart,rp,tmp1
	ldr \rp, =SYS_BASE
.endm

.macro senduart,rd,rx
	strb \rd, [\rx, #SYS_OUT_DATA]
.endm

.macro waituart,rd,rx
1001:	ldr \rd, [\rx, #SYS_INT_STATUS]
	tst \rd, #INT_HOST_IRQ
	bne 1001b
.endm

.macro busyuart,rd,rx
.endm

.macro	addruart_current, rx, tmp1, tmp2
	addruart	\rx, \tmp1
.endm

undef:
	ldr 	r0, =undef_message
	bl printascii
    b .
swi:
	ldr 	r0, =swi_message
	bl printascii
    b .
pref:
	ldr 	r0, =pref_message
	bl printascii
    b .
data:
	ldr 	r0, =data_message
	bl printascii
    b .
irq:
	ldr 	r0, =irq_message
	bl printascii
    b .
firq:
	ldr 	r0, =firq_message
	bl printascii
    b .

	
printascii:
	addruart_current r3, r1, r2
	b	2f
1:	waituart r2, r3
	senduart r1, r3
	busyuart r2, r3
	teq	r1, #'\n'
	moveq	r1, #'\r'
	beq	1b
2:	teq	r0, #0
	ldrneb	r1, [r0], #1
	teqne	r1, #0
	bne	1b
	mov	pc, lr

reset:
	ldr 	r0, =boot_message
	bl printascii
	b boot_linux
boot_message:
	.ascii	"\n==========================================\n"
	.ascii  "Shboot Bootloader Loading Linux"
	.asciz  "\n==========================================\n"
	.align

undef_message:
	.asciz "Undefined"
	.align
swi_message:
	.asciz "SWI"
	.align
pref_message:
	.asciz "Prefetch Abort"
	.align
data_message:
	.asciz "Data Abort"
	.align
irq_message:
	.asciz "Interrupt"
	.align
firq_message:
	.asciz "Fast Interrupt"
	.align

	
boot_linux:
	/* Copy oftree to RAM */
	@ldr	r0, =(DTB_DST)
	@ldr	r1, =(DTB_SRC)
	@ldr	r2, [r1, #4]

    @bl	memcpy

	/* boot Linux */
	mov	r0, #0
@	ldr	r1, =#0x12ab
	ldr	r2, =(DTB_ENTRY)
	mov	r3, #0
	mov	r4, #0
	mov	r5, #0
	mov	r6, #0

	ldr	r7, =(LINUX_ENTRY)
	mov 	pc, r7

memcpy:
	@ copies r2 bytes from r1 to r0 with r2 > 0
	ldrb	r3, [r1], #1
	strb	r3, [r0], #1
	subs	r2, r2, #1
	bhi	memcpy
	bx	lr
