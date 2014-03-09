/* Contains a minimal set of constants used to implement a static
   mapping for the registers used to access the UARTs. No constants
   with addresses or IRQ numbers for any other devices: they will be
   provided throug the Device Tree. */

#ifndef __SHMAC_H
#define __SHMAC_H

#define SHMAC_REGS_PHYS_BASE 0x0
#define SHMAC_REGS_VIRT_BASE IOMEM(0x0)
#define SHMAR_REGS_SIZE SZ_1M

#endif /* __SHMAC_H */
