#include "shboot.h"
	b 	reset

	
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
boot_message:
	.asciz	"Hello world"



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
	b	.
	ldr	r7, =(LINUX_ENTRY)
	mov 	pc, r7

memcpy:
	@ copies r2 bytes from r1 to r0 with r2 > 0
	ldrb	r3, [r1], #1
	strb	r3, [r0], #1
	subs	r2, r2, #1
	bhi	memcpy
	bx	lr
