/*
 * arch/arm/mach-shmac/include/mach/shmac.h
 *
 * Header file for SHMAC platform
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#ifndef __SHMAC_H
#define __SHMAC_H

// ============  address regions ===========================

#define RAM_BASE    0x00000000

#define BRAM0_BASE  0xF8000000
#define BRAM1_BASE  0xF9000000
#define BRAM2_BASE  0xFA000000
#define BRAM3_BASE  0xFB000000
#define BRAM4_BASE  0xFC000000
#define BRAM5_BASE  0xFD000000
#define BRAM6_BASE  0xFE000000
#define BRAM7_BASE  0xFF000000

#define TILE_BASE   0xFFFE0000

#define SYS_BASE    0xFFFF0000

#define BRAM_SIZE   16384

// ============  system registers ==========================

#define SYS_OUT_DATA    ((volatile uint32_t*)(SYS_BASE+0x00))
#define SYS_IN_DATA     ((volatile uint32_t*)(SYS_BASE+0x10))
#define SYS_INT_STATUS  ((volatile uint32_t*)(SYS_BASE+0x20))
#define SYS_TICKS       ((volatile uint32_t*)(SYS_BASE+0x30))
#define SYS_CPU_COUNT   ((volatile uint32_t*)(SYS_BASE+0x40))
#define SYS_READY       ((volatile uint32_t*)(SYS_BASE+0x50))

// ============  tile units ================================

#define TILE_REGS_BASE  (TILE_BASE+0x0000)
#define TIMER0_BASE     (TILE_BASE+0x1000)
#define TIMER1_BASE     (TILE_BASE+0x1100)
#define TIMER2_BASE     (TILE_BASE+0x1200)
#define INT_CTRL0_BASE  (TILE_BASE+0x2000)
#define INT_CTRL1_BASE  (TILE_BASE+0x2040)

// ============  tile registers  ===========================

#define TILEREG_CPUID     ((volatile uint32_t*)(TILE_REGS_BASE+0x000))
#define TILEREG_TILE_X    ((volatile uint32_t*)(TILE_REGS_BASE+0x004))
#define TILEREG_TILE_Y    ((volatile uint32_t*)(TILE_REGS_BASE+0x008))
#define TILEREG_DUMMY     ((volatile uint32_t*)(TILE_REGS_BASE+0x00c))

#define TIMER0_LOAD      ((volatile uint32_t*)(TIMER0_BASE+0x00))
#define TIMER0_VALUE     ((volatile uint32_t*)(TIMER0_BASE+0x04))
#define TIMER0_CTRL      ((volatile uint32_t*)(TIMER0_BASE+0x08))
#define TIMER0_CLR       ((volatile uint32_t*)(TIMER0_BASE+0x0c))

#define TIMER1_LOAD      ((volatile uint32_t*)(TIMER1_BASE+0x00))
#define TIMER1_VALUE     ((volatile uint32_t*)(TIMER1_BASE+0x04))
#define TIMER1_CTRL      ((volatile uint32_t*)(TIMER1_BASE+0x08))
#define TIMER1_CLR       ((volatile uint32_t*)(TIMER1_BASE+0x0c))

#define TIMER2_LOAD      ((volatile uint32_t*)(TIMER2_BASE+0x00))
#define TIMER2_VALUE     ((volatile uint32_t*)(TIMER2_BASE+0x04))
#define TIMER2_CTRL      ((volatile uint32_t*)(TIMER2_BASE+0x08))
#define TIMER2_CLR       ((volatile uint32_t*)(TIMER2_BASE+0x0c))

#define IC0_IRQ_STATUS      ((volatile uint32_t*)(INT_CTRL0_BASE+0x00))
#define IC0_IRQ_RAWSTAT     ((volatile uint32_t*)(INT_CTRL0_BASE+0x04))
#define IC0_IRQ_ENABLESET   ((volatile uint32_t*)(INT_CTRL0_BASE+0x08)) 
#define IC0_IRQ_ENABLECLR   ((volatile uint32_t*)(INT_CTRL0_BASE+0x0c)) 
#define IC0_INT_SOFTSET     ((volatile uint32_t*)(INT_CTRL0_BASE+0x10))
#define IC0_INT_SOFTCLEAR   ((volatile uint32_t*)(INT_CTRL0_BASE+0x14))
#define IC0_FIRQ_STATUS     ((volatile uint32_t*)(INT_CTRL0_BASE+0x20))  
#define IC0_FIRQ_RAWSTAT    ((volatile uint32_t*)(INT_CTRL0_BASE+0x24))  
#define IC0_FIRQ_ENABLESET  ((volatile uint32_t*)(INT_CTRL0_BASE+0x28))  
#define IC0_FIRQ_ENABLECLR  ((volatile uint32_t*)(INT_CTRL0_BASE+0x2c)) 

#define IC0_IRQ_STATUS_OFFSET      0x00
#define IC0_IRQ_RAWSTAT_OFFSET     0x04
#define IC0_IRQ_ENABLESET_OFFSET   0x08
#define IC0_IRQ_ENABLECLR_OFFSET   0x0c 
#define IC0_INT_SOFTSET_OFFSET     0x10
#define IC0_INT_SOFTCLEAR_OFFSET   0x14
#define IC0_FIRQ_STATUS_OFFSET     0x20  
#define IC0_FIRQ_RAWSTAT_OFFSET    0x24  
#define IC0_FIRQ_ENABLESET_OFFSET  0x28  
#define IC0_FIRQ_ENABLECLR_OFFSET  0x2c 

#define IC1_IRQ_STATUS      ((volatile uint32_t*)(INT_CTRL1_BASE+0x00))
#define IC1_IRQ_RAWSTAT     ((volatile uint32_t*)(INT_CTRL1_BASE+0x04))
#define IC1_IRQ_ENABLESET   ((volatile uint32_t*)(INT_CTRL1_BASE+0x08)) 
#define IC1_IRQ_ENABLECLR   ((volatile uint32_t*)(INT_CTRL1_BASE+0x0c)) 
#define IC1_INT_SOFTSET     ((volatile uint32_t*)(INT_CTRL1_BASE+0x10))
#define IC1_INT_SOFTCLEAR   ((volatile uint32_t*)(INT_CTRL1_BASE+0x14))
#define IC1_FIRQ_STATUS     ((volatile uint32_t*)(INT_CTRL1_BASE+0x20))  
#define IC1_FIRQ_RAWSTAT    ((volatile uint32_t*)(INT_CTRL1_BASE+0x24))  
#define IC1_FIRQ_ENABLESET  ((volatile uint32_t*)(INT_CTRL1_BASE+0x28))  
#define IC1_FIRQ_ENABLECLR  ((volatile uint32_t*)(INT_CTRL1_BASE+0x2c)) 

// ============  timer ctrl flags ==========================

#define TIMER_CTRL_ENABLE (1<<7)
#define TIMER_CTRL_PERIODIC (1<<6)
#define TIMER_CTRL_SCALE_256 0
#define TIMER_CTRL_SCALE_16 4
#define TIMER_CTRL_SCALE_1 8

// ============  interrupt bits ==========================

#define INT_MASK_SOFT 1
#define INT_MASK_HOST 2
#define INT_MASK_TIMER0 4
#define INT_MASK_TIMER1 8
#define INT_MASK_TIMER2 16

#endif
