//#include <kernel.h>

static struct map_desc shmac_io_desc[] __initdata = {
	{
		.virtual = (unsigned long) SHMAC_REGS_VIRT_BASE,
		.pfn = __phys_to_pfn (SHMAC_REGS_PHYS_BASE),
		.length = SHMAC_REGS_SIZE,
		.type = MT_DEVICE,
	},
};

void __init shmac_map_io(void){
	iotable_init(shmac_io_desc, ARRAY_SIZE(shmac_io_desc));
}

struct sys_timer shmac_timer = {
	.init = shmac_timer_init,
};

static void __init shmac_dt_init(void){
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static char const *shmac_dt_compat[] = {
	"shmac,shmac",
	NULL
};

DT_MACHINE_START(SHMAC_DT, "SHMAC")
        .init_irq = ircqchip_init,
	.map_io = shmac._map_io,
	.handle_irq = shmac_handle_irq,
	.init_machine = shmac_machine_init,
	.time = shmac_timer,
	.dt_compat = shmac_dt_compat,
	.restart = shmac_restart,
MACHINE_END

