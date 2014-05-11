#include <linux/kernel.h>
#include <linux/of_platform.h>

#include <asm/mach/arch.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>
#include <linux/clocksource.h>

static void __init shmac_init(void){
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static char const *shmac_dt_compat[] = {
	"shmac,shmac",
	NULL
};

DT_MACHINE_START(SHMAC_DT, "SHMAC")
	.init_machine = shmac_init,
	.dt_compat = shmac_dt_compat,
MACHINE_END

