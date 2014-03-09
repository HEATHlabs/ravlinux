#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/sched_clock.h>

#include <asm/cputype.h>

#define DEFAULT_TIMER	0
#define TIMER0_LOAD      0x00
#define TIMER0_VALUE     0x04
#define TIMER0_CTRL      0x08
#define TIMER0_CLR       0x0c

struct shmac_timer {
	void __iomem *control;
	void __iomem *clear;
	void __iomem *load;
	void __iomem *value;
	int match_mask;
	struct clock_event_device evt;
	struct irqaction act;
};

static irqreturn_t shmac_time_interrupt(int irq, void *dev_id)
{
	struct shmac_timer *timer = dev_id;
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

	base = of_iomap(node, 0);
	if (!base)
		panic("Can't remap registers");

	if (of_property_read_u32(node, "clock-frequency", &freq))
		panic("Can't read clock-frequency");

	system_clock = base + TIMER0_VALUE; //REG_COUNTER_LO;
	//TODO setup_sched_clock(bcm2835_sched_read, 32, freq);

	clocksource_mmio_init(base + TIMER0_VALUE, node->name,
		freq, 300, 32, clocksource_mmio_readl_up);

	irq = irq_of_parse_and_map(node, DEFAULT_TIMER);
	if (irq <= 0)
		panic("Can't parse IRQ");

	timer = kzalloc(sizeof(*timer), GFP_KERNEL);
	if (!timer)
		panic("Can't allocate timer struct\n");

	timer->control  = base + TIMER0_CTRL;
  timer->clear    = base + TIMER0_CLR;
  timer->load     = base + TIMER0_LOAD;
  timer->value    = base + TIMER0_VALUE;
  // TODO need to set evt.event_handler ?
  timer->evt.name = node->name;
	timer->evt.rating = 300;
	timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
	// timer->evt.set_mode = bcm2835_time_set_mode;
	// timer->evt.set_next_event = bcm2835_time_set_next_event;
	// timer->evt.cpumask = cpumask_of(0);
	timer->act.name = node->name;
	timer->act.flags = IRQF_TIMER | IRQF_SHARED;
	timer->act.dev_id = timer;
	timer->act.handler = shmac_time_interrupt;

	if (setup_irq(irq, &timer->act))
		panic("Can't set up timer IRQ\n");

	clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);

	pr_info("shmac: system timer (irq = %d)\n", irq);
}
CLOCKSOURCE_OF_DECLARE(bcm2835, "arm,shmac-system-timer",
			shmac_timer_init);

