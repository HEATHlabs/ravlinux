#include <linux/bitops.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/irqchip/versatile-fpga.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <asm/exception.h>
#include <asm/mach/irq.h>
#include "irqchip.h"
#include <asm/setup.h>

#define TILE_BASE           0xFFFE0000
#define INT_CTRL0_BASE      (TILE_BASE+0x2000)

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

/* Handle SHMAC irq */
asmlinkage void __exception_irq_entry shmac_handle_irq(struct pt_regs *regs)
{
	struct shmac_irq_data *s;
	int handled = 0;
	int irq;
	u32 status;

	s = &shmac_irq_device;
	while ((status  = readl(s->base + IRQ_STATUS))) {
		irq = ffs(status) - 1;
		handle_IRQ(irq_find_mapping(s->domain, irq), regs);
		handled = 1;
	}
}

static int shmac_irqdomain_map(struct irq_domain *d, unsigned int irq,
		irq_hw_number_t hwirq)
{
	struct shmac_irq_data *s = d->host_data;

  // Don't bother whith checking validity
	irq_set_chip_data(irq, s);
	irq_set_chip_and_handler(irq, &s->chip,
				handle_level_irq);
	set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	return 0;
}

// TODO ???
static struct irq_domain_ops shmac_irqdomain_ops = {
	.map = shmac_irqdomain_map,
	.xlate = irq_domain_xlate_onetwocell,
};

void __init shmac_irq_init(void __iomem *base, const char *name, int irq_start,
			  int parent_irq, u32 valid, struct device_node *node)
{
	struct shmac_irq_data *s;
	int i;

  early_print("I AM HEEERE\n");
	s = &shmac_irq_device;
	s->base = base;
	s->chip.name = name;
	s->chip.irq_ack = shmac_irq_mask;
	s->chip.irq_mask = shmac_irq_mask;
	s->chip.irq_unmask = shmac_irq_unmask;
	s->valid = valid;

	/* This will also allocate irq descriptors */
	s->domain = irq_domain_add_simple(node, fls(valid), irq_start,
					  &shmac_irqdomain_ops, s);

	/* This will allocate all valid descriptors in the linear case */
	for (i = 0; i < fls(valid); i++)
		if (valid & BIT(i)) {
			if (!irq_start)
				irq_create_mapping(s->domain, i);
			s->used_irqs++;
		}

	pr_info("SHMAC IRQ chip %d \"%s\" @ %p, %u irqs\n",
		shmac_irq_id, name, base, s->used_irqs);

	shmac_irq_id++;
}


#ifdef CONFIG_OF
int __init shmac_of_irq_init(struct device_node *node,
			    struct device_node *parent)
{
	void __iomem *base;
	u32 clear_mask;
	u32 valid_mask;

	if (WARN_ON(!node))
		return -ENODEV;

	base = of_iomap(node, 0);
	WARN(!base, "unable to map shmac irq registers\n");

	if (of_property_read_u32(node, "clear-mask", &clear_mask))
		clear_mask = 0;

	if (of_property_read_u32(node, "valid-mask", &valid_mask))
		valid_mask = 0;

	shmac_irq_init(base, node->name, 0, -1, valid_mask, node);

	writel(clear_mask, base + IRQ_ENABLE_CLEAR);
	writel(clear_mask, base + FIQ_ENABLE_CLEAR);

	return 0;
}
#endif

IRQCHIP_DECLARE(shmac, "shmac,shmac-intc", shmac_of_irq_init);

