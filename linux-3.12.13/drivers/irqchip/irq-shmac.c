#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <asm/exception.h>
#include "irqchip.h"
#include <linux/bitops.h>
#include <linux/irqchip/versatile-fpga.h>
#include <linux/irqdomain.h>
#include <linux/module.h>

#include <asm/setup.h>

/* The base of the IRQ-device */
static void __iomem *base;

/* IRQ-device offsets */
#define IRQ_STATUS		    0x00
#define IRQ_RAW_STATUS		0x04
#define IRQ_ENABLE_SET		0x08
#define IRQ_ENABLE_CLEAR	0x0c

/* IRQ-domain used for mapping between virtual IRQns 
 * and HW-IRQns */
static struct irq_domain *shmac_irq_domain;

/* The SHMAC IRQ does not use pending interrupts, so ack is not needed. */
static void shmac_irq_ack(struct irq_data *irqd){ }

/* Mask (turn off) an interrupt */
static void shmac_irq_mask(struct irq_data *d)
{
    /* Find the HW-IRQ-nr by looking at the domain */
    unsigned int irq = irqd_to_hwirq(d);

    unsigned int pos = ffs(irq)-1;

    /* Write to IRQ control register to mask interrupt */
    writel((1 << pos),base + IRQ_ENABLE_CLEAR);
}


static void shmac_irq_unmask(struct irq_data *d)
{
    unsigned int irq = irqd_to_hwirq(d);
    unsigned int pos = ffs(irq);
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
    int irq;
    u32 status;

    /* Read the status register in order to identify the interrupt source */
    status  = readl(base + IRQ_STATUS);

    /* Use ffs (find first non-zero bit) in order to find the hardware irq-number */
    irq = ffs(status) - 1;

    /* Call the appropriate handler by looking at the irq-domain */
    handle_IRQ(irq_find_mapping(shmac_irq_domain, irq), regs);
}

/* Callback used when a new interrupt source is registered.
 * This function will allocade a virtual IRQ-number and register
 * it within the irqdomain */
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

/* Initialize SHMAC irq controller. Is only called once */
int __init shmac_of_irq_init(struct device_node *node,
        struct device_node *parent)
{
    /* Extract base address from 'regs' property in DT tree*/
    base = of_iomap(node, 0);
    WARN(!base, "unable to map shmac irq registers\n");

    /* Set up IRQ-domain, this will create a 1-1 mapping between virtual 
     * irq-numbers used by the kernel, and the physical irq-numbers */
    shmac_irq_domain = irq_domain_add_linear(node, 32, &shmac_irqdomain_ops, NULL);
    shmac_irq_domain->hwirq_max = 10; 

    pr_info("SHMAC IRQ chip \"%s\" @ %p\n",
            node->name, base);

    /* Set interrupt handler */
    set_handle_irq(shmac_handle_irq);

    return 0;
}

/* Declare the IRQCHIP so that it can be mapped to the SHMAC DT (arch/arm/boot/dts/shmac.dtsi) */
IRQCHIP_DECLARE(shmac, "shmac,shmac-intc", shmac_of_irq_init);

