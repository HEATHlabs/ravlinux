#define SYS_BASE 0xffff0000
#define SYS_OUT_DATA 0x00
#define SYS_INT_STATUS 0x20
#define INT_HOST_IRQ 0x2

.macro addruart,rp,tmp
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
