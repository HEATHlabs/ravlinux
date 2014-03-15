#include <linux/bitops.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sched_clock.h>

#include <asm/irq.h>
#define DEFAULT_TIMER	0
#define TIMER_LOAD      0x00
#define TIMER_VALUE     0x04
#define TIMER_CTRL      0x08
#define TIMER_CLR       0x0c

#define TIMER_CTRL_ENABLE (1<<7)
#define TIMER_CTRL_PERIODIC (1<<6)
#define TIMER_CTRL_SCALE_256 0
#define TIMER_CTRL_SCALE_16 4
#define TIMER_CTRL_SCALE_1 8


struct shmac_timer {
	void __iomem *control;
	void __iomem *clear;
	void __iomem *load;
	void __iomem *value;
	int match_mask;
	struct clock_event_device evt;
	struct irqaction act;
};
static void __iomem *system_clock __read_mostly;

static u32 notrace shmac_sched_read(void)
{
	return readl_relaxed(system_clock);
}

static void shmac_time_set_mode(enum clock_event_mode mode,
                                       struct clock_event_device *evt_dev)
{
        switch (mode) {
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	default:
		WARN(1, "%s: unhandled event mode %d\n", __func__, mode);
		break;
	}
}

static int shmac_time_set_next_event(unsigned long event, struct 
                                     clock_event_device *evt_dev){
        struct shmac_timer *timer = container_of(evt_dev,
		struct shmac_timer, evt);
        writel_relaxed(60000, timer->value);
        writel_relaxed(TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC | TIMER_CTRL_SCALE_1, timer->control);
        return 0;
}

static irqreturn_t shmac_time_interrupt(int irq, void *dev_id)
{
    printk(".\n");
	struct shmac_timer *timer;
    timer = dev_id;
	void (*event_handler)(struct clock_event_device *);
	if (readl_relaxed(timer->control) & timer->match_mask) {
		writel_relaxed(timer->match_mask, timer->control);

		event_handler = ACCESS_ONCE(timer->evt.event_handler);
		if (event_handler)
			event_handler(&timer->evt);
		return IRQ_HANDLED;
	} else {
		return IRQ_NONE;
	}
}

static void __init shmac_timer_init(struct device_node *node)
{
	void __iomem *base;
	u32 freq;
	int irq;
	struct shmac_timer *timer;

        printk("!!!!!!!!!!!!!!!!! SHMAC TIMER INIT !!!!!!!!!!!!!!!!!!!!\n");
	base = of_iomap(node, 0);
	if (!base)
		panic("Can't remap registers");

	if (of_property_read_u32(node, "clock-frequency", &freq))
		panic("Can't read clock-frequency");
        printk("%s: %d\n", __FILE__, __LINE__);
	system_clock = base + TIMER_VALUE; //REG_COUNTER_LO;
	//DO setup_sched_clock(bcm2835_sched_read, 32, freq);
printk("%s: %d\n", __FILE__, __LINE__);
	clocksource_mmio_init(base + TIMER_VALUE, node->name,
		freq, 300, 32, clocksource_mmio_readl_up);
printk("%s: %d\n", __FILE__, __LINE__);
	irq = irq_of_parse_and_map(node, DEFAULT_TIMER);
	if (irq <= 0)
		panic("Can't parse IRQ");
printk("%s: %d\n", __FILE__, __LINE__);
	timer = kzalloc(sizeof(*timer), GFP_KERNEL);
	if (!timer)
		panic("Can't allocate timer struct\n");
printk("%s: %d\n", __FILE__, __LINE__);
	timer->control  = base + TIMER_CTRL;
        timer->clear    = base + TIMER_CLR;
        timer->load     = base + TIMER_LOAD;
        timer->value    = base + TIMER_VALUE;
        // TODO need to set evt.event_handler ?
        timer->evt.name = node->name;
	timer->evt.rating = 300;
	timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
	timer->evt.set_mode = shmac_time_set_mode;
	timer->evt.set_next_event = shmac_time_set_next_event;
	timer->evt.cpumask = cpumask_of(0);
	timer->act.name = node->name;
	timer->act.flags = IRQF_TIMER | IRQF_SHARED;
	timer->act.dev_id = timer;
	timer->act.handler = shmac_time_interrupt;
printk("%s: %d\n", __FILE__, __LINE__);
	if (setup_irq(irq, &timer->act))
		panic("Can't set up timer IRQ\n");
printk("%s: %d\n", __FILE__, __LINE__);
	clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);
printk("%s: %d\n", __FILE__, __LINE__);
	pr_info("shmac: system timer (irq = %d)\n", irq);
}
CLOCKSOURCE_OF_DECLARE(shmac, "shmac,shmac-timer",
			shmac_timer_init);

