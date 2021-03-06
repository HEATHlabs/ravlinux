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
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sched_clock.h>
#include <linux/jiffies.h>
#include <asm/irq.h>
#include <linux/clk.h>

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

#define SHMAC_TIMER_CLEAR(base) writel_relaxed(TIMER_INT_CLEAR, base + TIMER_CLR);
#define SHMAC_TIMER_STOP(base) writel_relaxed(TIMER_CTRL_DISABLE, base + TIMER_CTRL);
// the timer load register has an implicit left shift with 8, compensate for it here.
#define SHMAC_TIMER_SET_VALUE(value, base) writel_relaxed(value>>8, base + TIMER_LOAD )
#define SHMAC_TIMER_START(base)  \
        writel_relaxed(TIMER_CTRL_ENABLE | TIMER_CTRL_SCALE_1 | TIMER_CTRL_PERIODIC\
                        , base + TIMER_CTRL);
#define SHMAC_SYSTEM_TICK_COUNTER (const volatile void *)0xffff0030

struct shmac_clock_event_ddata {
        struct clock_event_device evtdev;
        void __iomem *base;
};

static u32 ticks_per_jiffy;
static u32 source_base;

// The timer read register has an implicit right shift with 8, compensate for it here
cycle_t shmac_clocksource_mmio_readl_down(struct clocksource *c){
        return clocksource_mmio_readw_down(c)<<8;
}

static void shmac_clock_event_set_mode(enum clock_event_mode mode,
                struct clock_event_device *evtdev)
{
        struct shmac_clock_event_ddata *ddata =
                container_of(evtdev, struct shmac_clock_event_ddata, evtdev);
        switch (mode) {
                case CLOCK_EVT_MODE_ONESHOT:
                        SHMAC_TIMER_STOP(ddata->base);
                        SHMAC_TIMER_START(ddata->base);
                        break;
                case CLOCK_EVT_MODE_PERIODIC:
                        SHMAC_TIMER_STOP(ddata->base);
                        SHMAC_TIMER_SET_VALUE(ticks_per_jiffy,ddata->base);
                        SHMAC_TIMER_START(ddata->base);
                        break;
                case CLOCK_EVT_MODE_UNUSED:
                        break;
                case CLOCK_EVT_MODE_SHUTDOWN:
                       SHMAC_TIMER_STOP(ddata->base);
                        break;
                case CLOCK_EVT_MODE_RESUME:
                        break;
                default:
                        WARN(1, "%s: unhandled event mode %d\n", __func__, mode);
                        break;
        }
}

static int shmac_clock_event_set_next_event(unsigned long event, struct 
                clock_event_device *evtdev){
	struct shmac_clock_event_ddata *ddata =
		container_of(evtdev, struct shmac_clock_event_ddata, evtdev);

        SHMAC_TIMER_CLEAR(ddata->base);
        SHMAC_TIMER_SET_VALUE(event, ddata->base);
        SHMAC_TIMER_START(ddata->base);
        return 0;
}

static irqreturn_t shmac_time_interrupt(int irq, void *dev_id)
{
	struct shmac_clock_event_ddata *ddata = dev_id;
        SHMAC_TIMER_CLEAR(ddata->base);
        ddata->evtdev.event_handler(&ddata->evtdev);

        return IRQ_HANDLED;

}

static struct shmac_clock_event_ddata clock_event_ddata = {
        .evtdev = {
                .name = "shmac clockevent",
                .features = CLOCK_EVT_FEAT_PERIODIC,
                .set_mode = shmac_clock_event_set_mode,
                .set_next_event = shmac_clock_event_set_next_event,
                .rating = 200,
        },
};

static struct irqaction shmac_clock_event_irq = {
        .name = "shmac clockevent",
        .flags = IRQF_TIMER,
        .handler = shmac_time_interrupt,
        .dev_id = &clock_event_ddata,
};

static u32 notrace shmac_sched_clock_read(void)
{
        return readl(SHMAC_SYSTEM_TICK_COUNTER);
}

static void __init shmac_clocksource_init(struct device_node *node)
{
        void __iomem *base;
        u32 freq;
        

        base = of_iomap(node, 0);
        source_base = (u32) base;
        if (!base)
                panic("Can't remap registers");

        if (of_property_read_u32(node, "clock-frequency", &freq))
                panic("Can't read clock-frequency");

        SHMAC_TIMER_CLEAR(base);
        SHMAC_TIMER_START(base);
        
        setup_sched_clock(shmac_sched_clock_read, 32, freq/1024);
        /* Schedule a clock read from system tick counter instead of using the
           timer value as a clocksource.
   
        if(clocksource_mmio_init(base + TIMER_VALUE, "SHMAC clocksource",
                               freq, 200, 24, shmac_clocksource_mmio_readl_down))
                panic("Can't register clocksource\n");
        */
        pr_info("SHMAC clocksource init done\n");
       
}

static void __init shmac_clockevent_init(struct device_node *np)
{
        void __iomem *base;
        u32 freq;
        int irq;

        base = of_iomap(np, 0);
                if (!base)
                panic("Can't remap registers");

        if (of_property_read_u32(np, "clock-frequency", &freq))
                panic("Can't read clock-frequency");

        ticks_per_jiffy = DIV_ROUND_UP(freq, HZ);

        irq = irq_of_parse_and_map(np, DEFAULT_TIMER);
        if (irq <= 0)
                panic("Can't parse IRQ");


        if (setup_irq(irq, &shmac_clock_event_irq))
                panic("Can't set up timer IRQ\n");

        clock_event_ddata.base = base;
        clockevents_config_and_register(&clock_event_ddata.evtdev, freq, 0xf, 0xffff);
        
        pr_info("SHMAC clockevent init done\n");
}

static void __init shmac_timer_init(struct device_node *node)
{
        static int has_clocksource, has_clockevent;

        if(!has_clockevent)
        {
                shmac_clockevent_init(node);
                has_clockevent = 1;
                return;
        }

        if(!has_clocksource)
        {
                shmac_clocksource_init(node);
                has_clocksource = 1;
                return;
        }
}

CLOCKSOURCE_OF_DECLARE(shmac, "shmac,shmac-timer",
                shmac_timer_init);

