/*
 * arch/arm/mach-shmac/include/mach/entry-macro.S
 *
 * Low-level IRQ helper macros for SHMAC 
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <mach/shmac.h>
	.macro  get_irqnr_preamble, base, tmp
        ldr     \base, =INT_CTRL0_BASE
        .endm

        .macro  get_irqnr_and_base, irqnr, irqstat, base, tmp
       
	@ Check if there is any interrupt bit set
	ldr	\irqstat, [\base, #IC0_IRQ_STATUS_OFFSET]
        cmp     \irqstat, #0
        beq     1002f
	
                @ ok at least 1 interrupt bit is set, find it
        mov     \tmp, #0
	mov     \base, #1
        mov     \irqnr, #0
1001:
        add     \tmp, \tmp, \base, lsl \tmp
        add     \irqnr, \irqnr, #1
        cmp     \irqnr, #32
        beq     1002f
        tst     \tmp, \irqstat
        beq     1001b
        sub     \irqnr, \irqnr, #1
1002:
        .endm
