
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <asm/exception.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>

#include "irqchip.h"

#include <linux/bitops.h>
#include <linux/irqchip/versatile-fpga.h>
#include <linux/irqdomain.h>
#include <linux/module.h>

#include <asm/setup.h>

#define TILE_BASE           0xFFFE0000
#define INT_CTRL0_BASE      ((u32*)TILE_BASE+0x2000)
#define SYS_BASE            0xFFFF0000
#define SYS_IN_DATA         ((volatile u32*)(SYS_BASE+0x10))

#define IRQ_STATUS		    0x00
#define IRQ_RAW_STATUS		0x04
#define IRQ_ENABLE_SET		0x08
#define IRQ_ENABLE_CLEAR	0x0c
#define INT_SOFT_SET		  0x10
#define INT_SOFT_CLEAR		0x14
#define FIQ_STATUS		    0x20
#define FIQ_RAW_STATUS		0x24
#define FIQ_ENABLE		    0x28
#define FIQ_ENABLE_SET		0x28
#define FIQ_ENABLE_CLEAR	0x2C
#define IC0_IRQ_STATUS      ((volatile u32*)(INT_CTRL0_BASE+0x00))
#define IC0_IRQ_RAWSTAT     ((volatile u32*)(INT_CTRL0_BASE+0x04))
#define IC0_IRQ_ENABLESET   ((volatile u32*)(INT_CTRL0_BASE+0x08)) 
#define IC0_IRQ_ENABLECLR   ((volatile u32*)(INT_CTRL0_BASE+0x0c)) 
#define IC0_INT_SOFTSET     ((volatile u32*)(INT_CTRL0_BASE+0x10))
#define IC0_INT_SOFTCLEAR   ((volatile u32*)(INT_CTRL0_BASE+0x14))
#define IC0_FIRQ_STATUS     ((volatile u32*)(INT_CTRL0_BASE+0x20))  
#define IC0_FIRQ_RAWSTAT    ((volatile u32*)(INT_CTRL0_BASE+0x24))  
#define IC0_FIRQ_ENABLESET  ((volatile u32*)(INT_CTRL0_BASE+0x28))  
#define IC0_FIRQ_ENABLECLR  ((volatile u32*)(INT_CTRL0_BASE+0x2c)) 

#define TIMER0_BASE     (TILE_BASE+0x1000)
#define TIMER0_LOAD      ((volatile u32*)(TIMER0_BASE+0x00))
#define TIMER0_VALUE     ((volatile u32*)(TIMER0_BASE+0x04))
#define TIMER0_CTRL      ((volatile u32*)(TIMER0_BASE+0x08))
#define TIMER0_CLR       ((volatile u32*)(TIMER0_BASE+0x0c))
#define TIMER_CTRL_ENABLE (1<<7)
#define TIMER_CTRL_PERIODIC (1<<6)
#define TIMER_CTRL_SCALE_256 0
#define TIMER_CTRL_SCALE_16 4
#define TIMER_CTRL_SCALE_1 8
#define INT_MASK_SOFT 1
#define INT_MASK_HOST 2
#define INT_MASK_TIMER0 4
#define INT_MASK_TIMER1 8
#define INT_MASK_TIMER2 16

static void __iomem *base;
static struct irq_domain *shmac_irq_domain;
static u32 valid_mask;
/* We only use one of the IRQ-devices on SHMAC */
static int shmac_irq_id;

static void shmac_irq_ack(struct irq_data *irqd)
{
  printk("SHMACK (empty)\n");
}

static void shmac_irq_mask(struct irq_data *d)
{
  unsigned int irq = irqd_to_hwirq(d);
  unsigned int pos = ffs(irq)-1;
  u32 addr = base + IRQ_ENABLE_CLEAR;
  printk("SHMIRQ: MASK nr: %d, at pos: %d, to addr 0x%x", irq,pos,addr); 
	writel((1 << pos),IC0_IRQ_ENABLECLR);
}


static void shmac_irq_unmask(struct irq_data *d)
{
  unsigned int irq = irqd_to_hwirq(d);
  unsigned int pos = ffs(irq);
  printk("SHMIRQ: UNMASK nr: 0x%x\n", irq); 
	writel((1 << pos), base + IRQ_ENABLE_SET);
}

static struct irq_chip shmac_irq_chip= {
	.name		= "shmac_irq",
	.irq_ack	= shmac_irq_ack,
	.irq_mask	= shmac_irq_mask,
	.irq_unmask	= shmac_irq_unmask,
};

/* Handle SHMAC irq */
static asmlinkage void __exception_irq_entry shmac_handle_irq(struct pt_regs *regs)
{
	int handled = 0;
	int irq;

	u32 status;
  u32 data   = *SYS_IN_DATA;
	status  = readl(base + IRQ_STATUS);
  irq = ffs(status) - 1;
  printk("Softirq: %d\n", irq_find_mapping(shmac_irq_domain, irq));
  handle_IRQ(irq_find_mapping(shmac_irq_domain, irq), regs);
  handled = 1;
}

static int shmac_irqdomain_map(struct irq_domain *d, unsigned int irq,
		irq_hw_number_t hwirq)
{
  printk("IRQ: Callback irqnr: %d, assigned with:%d\n",(int) hwirq,irq);
	irq_set_chip_and_handler(irq, &shmac_irq_chip, handle_level_irq);
	set_irq_flags(irq, IRQF_VALID);
	return 0;
}

/* Set up the options given to the irqdomain_add
 * function. .map is requierd.
 */
static struct irq_domain_ops shmac_irqdomain_ops = {
	.map = shmac_irqdomain_map,
	.xlate = irq_domain_xlate_onecell,
};

void __init shmac_irq_init(void __iomem *base, const char *name,
			  int parent_irq, u32 valid, struct device_node *node)
{
	int i;
  int used_irqs = 0;

	/* This will also allocate irq descriptors */
	shmac_irq_domain = irq_domain_add_linear(node, 32, &shmac_irqdomain_ops, NULL);
  shmac_irq_domain->hwirq_max = 10; // TODO necessary ?

	/* This will allocate all valid descriptors in the linear case */
	for (i = 0; i < fls(valid); i++)
		if (valid & BIT(i)) {
			irq_create_mapping(shmac_irq_domain, i);
      used_irqs++;
		}

	pr_info("SHMAC IRQ chip %d \"%s\" @ %p, %u irqs\n",
		shmac_irq_id, name, base, used_irqs);
}


int __init shmac_of_irq_init(struct device_node *node,
			    struct device_node *parent)
{
	valid_mask = 0x6;

  base = 0xfffe2000; 
  // WARN(!base, "unable to map shmac irq registers\n");

  /* Set up IRQ-domain */
  shmac_irq_init(base, node->name, -1, valid_mask, node);

  /* Enable the interrupts */
  writel((1<<1), base + IRQ_ENABLE_SET);

  set_handle_irq(shmac_handle_irq);

	return 0;
}

IRQCHIP_DECLARE(shmac, "shmac,shmac-intc", shmac_of_irq_init);

