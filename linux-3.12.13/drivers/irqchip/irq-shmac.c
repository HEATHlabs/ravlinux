
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
#define INT_CTRL0_BASE      ((uint32_t*)TILE_BASE+0x2000)
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


/**
 * struct shmac_irq_data - irq data container for the SHMAC IRQ controller
 * @base: memory offset in memory
 * @chip: chip container for this instance
 * @domain: IRQ domain for this instance
 * @valid: mask for valid IRQs on this controller
 * @used_irqs: number of active IRQs on this controller
 */

struct shmac_irq_data {
	void __iomem *base;
	struct irq_chip chip;
	u32 valid;
	struct irq_domain *domain;
	u8 used_irqs;
};

/* We only use one of the IRQ-devices on SHMAC */
static struct shmac_irq_data shmac_irq_device;
static int shmac_irq_id;

/* also acts as ACK */
static void shmac_irq_mask(struct irq_data *d)
{
	struct shmac_irq_data *s = irq_data_get_irq_chip_data(d);
	u32 mask = 1 << d->hwirq; 

	writel(mask, s->base + IRQ_ENABLE_CLEAR);
}


static void shmac_irq_unmask(struct irq_data *d)
{
	struct shmac_irq_data *s = irq_data_get_irq_chip_data(d);
	u32 mask = 1 << d->hwirq;

	writel(mask, s->base + IRQ_ENABLE_SET);
}

/* Handle SHMAC irq */
static asmlinkage void __exception_irq_entry shmac_handle_irq(struct pt_regs *regs)
{
	int handled = 0;
	int irq;
	u32 status = *SYS_IN_DATA;
  struct shmac_irq_data *s;

  printk("IRQ, status: 0x%x\n",status);
	writel(0xffffffff, IC0_IRQ_ENABLECLR);
	s = &shmac_irq_device;
	while ((status  = readl(IC0_IRQ_STATUS))) {
		irq = ffs(status) - 1;
		handle_IRQ(irq_find_mapping(s->domain, irq), regs);
		handled = 1;
	}
}


static void shmac_irq_handle(unsigned int irq, struct irq_desc *desc)
{
	struct shmac_irq_data *s = irq_desc_get_handler_data(desc);
	u32 status = readl(s->base + IRQ_STATUS);

	if (status == 0) {
		do_bad_IRQ(irq, desc);
		return;
	}

	do {
		irq = ffs(status) - 1;
		status &= ~(1 << irq);
		generic_handle_irq(irq_find_mapping(s->domain, irq));
	} while (status);
}

static int shmac_irqdomain_map(struct irq_domain *d, unsigned int irq,
		irq_hw_number_t hwirq)
{
	struct shmac_irq_data *s = d->host_data;
  printk("IRQ: Callback irqnr: %d, assigned with:%d\n",(int) hwirq,irq);
  // Don't bother whith checking validity
	irq_set_chip_data(irq, s);
	irq_set_chip_and_handler(irq, &s->chip,
				handle_level_irq);
	set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
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
	struct shmac_irq_data *s;
	int i;
  
	s = &shmac_irq_device;
	s->base = base;
	s->chip.name = name;
	s->chip.irq_ack = shmac_irq_mask;
	s->chip.irq_mask = shmac_irq_mask;
	s->chip.irq_unmask = shmac_irq_unmask;
	s->valid = valid;

	if (parent_irq != -1) {
		irq_set_handler_data(parent_irq, s);
		irq_set_chained_handler(parent_irq, shmac_irq_handle);
	}

	/* This will also allocate irq descriptors */
	s->domain = irq_domain_add_linear(node, fls(valid), &shmac_irqdomain_ops, s);

	/* This will allocate all valid descriptors in the linear case */
	for (i = 0; i < fls(valid); i++)
		if (valid & BIT(i)) {
			irq_create_mapping(s->domain, i);
			s->used_irqs++;
		}


  printk("IRQ: hwirq: %d, irq: %d\n",(int)irq_find_mapping(s->domain, 16),16);
  printk("IRQ: hwirq: %d, irq: %d\n",(int)irq_find_mapping(s->domain, 17),17);
	writel(s->valid, base + IRQ_ENABLE_SET);
  i = * (int*) (base + IRQ_ENABLE_SET);
  printk("IRQ: Enableset: 0x%x\n", i);
  printk("IRQ: valid: 0x%x\n", valid);
	pr_info("SHMAC IRQ chip %d \"%s\" @ %p, %u irqs\n",
		shmac_irq_id, name, base, s->used_irqs);

}


int __init shmac_of_irq_init(struct device_node *node,
			    struct device_node *parent)
{
	void __iomem *base;
	u32 valid_mask;

  base = of_iomap(node, 0);
  WARN(!base, "unable to map shmac irq registers\n");

	if (of_property_read_u32(node, "valid-mask", &valid_mask))
		valid_mask = 0;
  
  printk("VALID mask: 0x%x\n", valid_mask);

  /* Set up IRQ-domain */
	shmac_irq_init(base, node->name, -1, valid_mask, node);

  /* Clear all pending interrupts */
	writel(0xffffffff, base + IRQ_ENABLE_CLEAR);

  /* Enable the interrupts */
  writel(valid_mask, base + IRQ_ENABLE_SET);

	set_handle_irq(shmac_handle_irq);

	return 0;
}

IRQCHIP_DECLARE(shmac, "shmac,shmac-intc", shmac_of_irq_init);

