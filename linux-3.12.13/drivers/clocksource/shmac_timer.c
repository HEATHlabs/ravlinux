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
#include <linux/jiffies.h>
#include <asm/irq.h>
#define DEFAULT_TIMER	0
#define TIMER_LOAD      0x00
#define TIMER_VALUE     0x04
#define TIMER_CTRL      0x08
#define TIMER_CLR       0x0c

#define TIMER_CTRL_ENABLE       (1<<7)
#define TIMER_CTRL_DISABLE      (0<<7)
#define TIMER_CTRL_PERIODIC     (1<<6)
#define TIMER_CTRL_SCALE_256    0
#define TIMER_CTRL_SCALE_16     4
#define TIMER_CTRL_SCALE_1      8
#define TIMER_INT_CLEAR         1


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

static u32 ticks_per_jiffy;

static void shmac_timer_stop(struct shmac_timer* timer)
{
        writel_relaxed(TIMER_CTRL_DISABLE, timer->control);
}

static void shmac_timer_start(struct shmac_timer* timer, bool periodic)
{
        writel_relaxed(TIMER_CTRL_ENABLE | TIMER_CTRL_SCALE_1
                | (periodic ? TIMER_CTRL_PERIODIC : 0), timer->control);
}

static void shmac_timer_clear(struct shmac_timer* timer)
{
        writel_relaxed(TIMER_INT_CLEAR, timer->clear);
}

static void shmac_timer_set_value(struct shmac_timer* timer, unsigned long val)
{
        writel_relaxed(val, timer->load);
}

static u32 notrace shmac_sched_read(void)
{
	return readl(system_clock);
}

static void shmac_time_set_mode(enum clock_event_mode mode,
        struct clock_event_device *evt_dev)
{
    struct shmac_timer *timer = container_of(evt_dev, struct shmac_timer, evt);
    printk("CHANGING MODE: ");
    switch (mode) {
        case CLOCK_EVT_MODE_ONESHOT:
            shmac_timer_stop(timer);
            shmac_timer_start(timer, false);
            printk(" ONESHOT\n");
            break;
        case CLOCK_EVT_MODE_PERIODIC:
                printk("HZ: %d\n", HZ);
            shmac_timer_stop(timer);
            shmac_timer_set_value(timer, ticks_per_jiffy>>8);
            shmac_timer_start(timer, true);
            printk(" PERIODIC\n");
            break;
        case CLOCK_EVT_MODE_UNUSED:
                printk("UNUSED\n");
                break;
        case CLOCK_EVT_MODE_SHUTDOWN:
                printk("SHUTDOWN\n");
                break;
        case CLOCK_EVT_MODE_RESUME:
            printk("CV\n");
            break;
        default:
            WARN(1, "%s: unhandled event mode %d\n", __func__, mode);
            break;
    }
}

static int shmac_time_set_next_event(unsigned long event, struct 
                                     clock_event_device *evt_dev){
        struct shmac_timer *timer = container_of(evt_dev, struct shmac_timer, evt);
        printk(",");
      
        shmac_timer_clear(timer);
        shmac_timer_set_value(timer, event);
        shmac_timer_start(timer, false);
        return 0;
}

static irqreturn_t shmac_time_interrupt(int irq, void *dev_id)
{
        struct shmac_timer *timer = dev_id;
	void (*event_handler)(struct clock_event_device *) = timer->evt.event_handler;

        printk(".");
        shmac_timer_clear(timer);
        event_handler(&timer->evt);

        return IRQ_HANDLED;
	
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

	system_clock = base + TIMER_VALUE; 
	setup_sched_clock(shmac_sched_read, 32, freq);

	if(clocksource_mmio_init(base + TIMER_VALUE, node->name,
                                 freq, 300, 32, clocksource_mmio_readl_up))
                panic("Can't register clocksource\n");

	irq = irq_of_parse_and_map(node, DEFAULT_TIMER);
	if (irq <= 0)
		panic("Can't parse IRQ");

	timer = kzalloc(sizeof(*timer), GFP_KERNEL);
	if (!timer)
		panic("Can't allocate timer struct\n");

        timer->control  = base + TIMER_CTRL;
        timer->clear    = base + TIMER_CLR;
        timer->load     = base + TIMER_LOAD;
        timer->value    = base + TIMER_VALUE;
        timer->match_mask = BIT(DEFAULT_TIMER);

        timer->evt.name = node->name;
	timer->evt.rating = 300;
	timer->evt.features = CLOCK_EVT_FEAT_ONESHOT | CLOCK_EVT_FEAT_PERIODIC;
	timer->evt.set_mode = shmac_time_set_mode;
	timer->evt.set_next_event = shmac_time_set_next_event;
	timer->evt.cpumask = cpumask_of(0);


	timer->act.name = node->name;
	timer->act.flags = IRQF_TIMER | IRQF_SHARED | IRQF_IRQPOLL;
	timer->act.dev_id = timer;
	timer->act.handler = shmac_time_interrupt;

        ticks_per_jiffy = DIV_ROUND_UP(freq, HZ);

	if (setup_irq(irq, &timer->act))
		panic("Can't set up timer IRQ\n");

	clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);

	pr_info("shmac: system timer (irq = %d)\n", irq);
}
CLOCKSOURCE_OF_DECLARE(shmac, "shmac,shmac-timer",
			shmac_timer_init);

