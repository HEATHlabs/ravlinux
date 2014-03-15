#if defined(CONFIG_SERIAL_SHMAC_UART_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial_core.h>
#include <linux/tty_flip.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include <mach/shmac.h>

#define DRIVER_NAME "shmac-uart"
#define DEV_NAME "ttyshmc"

#define UART_NR 13

#define UARTn_STATUS 0x20
#define UARTn_STATUS_TXBUSY 0x2
#define UARTn_TXDATA 0x0

struct shmac_uart_port {
	struct uart_port port;
};


static void shmac_uart_write32(struct shmac_uart_port *shmac_port,
		u32 value, unsigned offset)
{
	writel_relaxed(value, shmac_port->port.membase + offset);
}

static u32 shmac_uart_read32(struct efm32_uart_port *shmac_port,
		unsigned offset)
{
	return readl_relaxed(shmac_port->port.membase + offset);
}

/* See Documentation/serial/driver for implementation details */

static unsigned int shmac_uart_tx_empty(struct uart_port *port)
{
	return TIOCSER_TEMT;
}

static void shmac_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	/* do nothing */
}

static unsigned int shmac_uart_get_mctrl(struct uart_port *port)
{
	/* Should indicate signal is permanently active */
	return TIOCM_CAR | TIOCM_CTS | TIOCM_DSR;
}

static void shmac_uart_stop_tx(struct uart_port *port)
{
	/* Stop transmitting characters */
}

static void shmac_uart_start_tx(struct uart_port *port)
{
	/* Start transmitting characters */
}

static void shmac_uart_stop_rx(struct uart_port *port)
{
	/* Stop receiving characters, the port is in the process of being closed */
}

static void shmac_uart_enable_ms(struct uart_port *port)
{
	/* Enable the modem status interrupts */
}

static void shmac_uart_break_ctl(struct uart_port *port, int ctl)
{
	/* Control the transission of a break signal */
}

static int shmac_uart_startup(struct uart_port *port)
{
	// TODO
	return 0;
}

static void shmac_uart_shutdown(struct uart_port *port)
{
	// TODO
}

static void shmac_uart_set_termios(struct uart_port *port, struct ktermios *new, struct ktermios *old)
{
	// TODO
	/* Change the port parameters */
}

static const char * shmac_uart_type(struct uart_port *port)
{
	return port->type == PORT_SHMACUART ? "shmac-uart" : NULL;
}

static void shmac_uart_release_port(struct uart_port *port)
{
	/* Release memory and io requested by port*/
	// TODO
}

static int shmac_uart_request_port(struct uart_port *port)
{
	/* Request memory and io for port, return EBUSY on failure*/
	// TODO
	return 0;
}

static void shmac_uart_config_port(struct uart_port *port, int type)
{
	/* Perform autoconfiguration */
	if(type & UART_CONFIG_TYPE && !shmac_uart_request_port(port))
		port->type = PORT_SHMACUART;
}

static int shmac_uart_verify_port(struct uart_port *port, struct serial_struct *serinfo )
{
	if(serinfo->type != PORT_UNKNOWN && serinfo->type != PORT_SHMACUART)
		return -EINVAL;
	return 0;
}

static struct uart_ops shmac_uart_port_ops = {
	.tx_empty = shmac_uart_tx_empty,
	.set_mctrl = shmac_uart_set_mctrl,
	.get_mctrl = shmac_uart_get_mctrl,
	.stop_tx = shmac_uart_stop_tx,
	.start_tx = shmac_uart_start_tx,
	.stop_rx = shmac_uart_stop_rx,
	.enable_ms = shmac_uart_enable_ms,
	.break_ctl = shmac_uart_break_ctl,
	.startup = shmac_uart_startup,
	.shutdown = shmac_uart_shutdown,
	.set_termios = shmac_uart_set_termios,
	.type = shmac_uart_type,
	.release_port = shmac_uart_release_port,
	.request_port = shmac_uart_request_port,
	.config_port = shmac_uart_config_port,
	.verify_port = shmac_uart_verify_port,
};

static struct shmac_uart_port *shmac_uart_ports[1];

#ifdef CONFIG_SERIAL_SHMAC_UART_CONSOLE
static void shmac_console_putchar(struct uart_port *port, int ch)
{
	struct shmac_uart_port *usp = (struct shmac_uart_port *)port;
        u32 status;

	while (true){
                status = shmac_uart_read32(usp->port.membase + UARTn_STATUS);
                if(status & UARTn_STATUS_TXBUSY)
                        break;
        }
	shmac_uart_write32(ch, usp->port.membase + UARTn_TXDATA);
}

static void
shmac_uart_console_write(struct console *co, const char *s, unsigned int count)
{
	struct shmac_uart_port *shmac_port = shmac_uart_ports[co->index];
	uart_console_write(&shmac_port->port, s, count, shmac_console_putchar);
}

static int shmac_uart_console_setup(struct console *co, char *options)
{
	return 0;
}

static struct uart_driver shmac_uart_reg;

static struct console shmac_uart_console = {
	.name = DEV_NAME,
	.write = shmac_uart_console_write,
	.device = uart_console_device,
	.setup = shmac_uart_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &shmac_uart_reg,
};

#else
#define shmac_uart_console (*(struct console *)NULL)
#endif /*ifdef CONFIG_SERIAL_SHMAC_UART_CONSOLE */

static struct uart_driver shmac_uart_reg = {
	.owner = THIS_MODULE,
	.driver_name = DRIVER_NAME,
	.dev_name = DEV_NAME,
	.nr = ARRAY_SIZE(shmac_uart_ports),
	.cons = &shmac_uart_console,
};

static int shmac_uart_probe_dt(struct platform_device *pdev, struct shmac_uart_port *shmac_port)
{
	struct device_node *np = pdev->dev.of_node;
	u32 location;
	int ret;
	if(!np)
		return 1;
	ret = of_property_read_u32(np, "location", &location);
	if(!ret){
		if(location > ARRAY_SIZE(shmac_uart_ports) ){
			dev_err(&pdev->dev, "invalid location\n");
			return -EINVAL;
		}
		dev_dbg(&pdev->dev, "using location %u\n", location);
		//shmac_port->pdata.location = location;
	} else {
		location = 0;
		dev_dbg(&pdev->dev, "fall back to location 0\n");
	}
	ret = of_alias_get_id(np, "serial");
	if(ret < 0){
		dev_err(&pdev->dev, "failed to get alial id: %d\n", ret);
		return ret;
	}
	return 0;
}

static int shmac_uart_probe(struct platform_device *pdev)
{
	struct shmac_uart_port *shmac_port;
	struct resource *res;
	int ret, line;
        
	shmac_port = devm_kzalloc(&pdev->dev, sizeof(struct shmac_uart_port),
			   GFP_KERNEL);
	if (shmac_port == NULL) {
		dev_dbg(&pdev->dev, "failed to allocate private data\n");
		return -ENOMEM;
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM,0);
	if(res == NULL){
		dev_dbg(&pdev->dev, "failed to get base address\n");
		kfree(shmac_port);
		return -ENODEV;
	}
	
	ret = platform_get_irq(pdev,1);
	if(ret <= 0){
		dev_dbg(&pdev->dev, "failed to get rx irq\n");
		kfree(shmac_port);
		return ret;
	}
	shmac_port->port.irq = ret;

	shmac_port->port.dev = &pdev->dev;
	shmac_port->port.mapbase = res->start;
	shmac_port->port.type = PORT_SHMACUART;
	shmac_port->port.iotype = UPIO_MEM32;
	shmac_port->port.fifosize = 2;
	shmac_port->port.ops = &shmac_uart_port_ops;
	shmac_port->port.flags = UPF_BOOT_AUTOCONF;

	ret = shmac_uart_probe_dt(pdev, shmac_port);
	if(ret > 0){
		/* Something wrong with device tree */
		return ret;
	}

	line = shmac_port->port.line;
	if(line >= 0 && line < ARRAY_SIZE(shmac_uart_ports))
		shmac_uart_ports[line] = shmac_port;

	ret = uart_add_one_port(&shmac_uart_reg, &shmac_port->port);
	if(ret){
		dev_dbg(&pdev->dev, "failed to add port: %d\n",ret);
		if(line >= 0 && line < ARRAY_SIZE(shmac_uart_ports))
			shmac_uart_ports[line] = NULL;

		kfree(shmac_port);
		return ret;
	} 
	
	platform_set_drvdata(pdev, shmac_port);

	return 0;
}

static int shmac_uart_remove(struct platform_device *pdev)
{
	struct shmac_uart_port *shmac_port = platform_get_drvdata(pdev);
	unsigned int line = shmac_port->port.line;

	uart_remove_one_port(&shmac_uart_reg, &shmac_port->port);

	if (line >= 0 && line < ARRAY_SIZE(shmac_uart_ports))
		shmac_uart_ports[line] = NULL;

	kfree(shmac_port);

	return 0;
}

static const struct of_device_id shmac_uart_dt_ids[] = {
	{
		.compatible = "shmac,shmac-uart",
	}, {
		/* sentinel */
	}
};

MODULE_DEVICE_TABLE(of, shmac_uart_dt_ids);

static struct platform_driver shmac_uart_driver = {
	.probe = shmac_uart_probe,
	.remove = shmac_uart_remove,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = shmac_uart_dt_ids,
	},
};

static int __init shmac_serial_init(void)
{
	int ret;
	printk("\nSHMAC SERIAL_CONSOLE init\n");
        while(true);
	ret = uart_register_driver(&shmac_uart_reg);
	if (ret)
		return ret;

	ret = platform_driver_register(&shmac_uart_driver);
	if (ret)
		uart_unregister_driver(&shmac_uart_reg);

	return ret;
}

static void __exit shmac_serial_exit(void)
{
	platform_driver_unregister(&shmac_uart_driver);
	uart_unregister_driver(&shmac_uart_reg);
}

module_init(shmac_serial_init);
module_exit(shmac_serial_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("Joakim Andersson");
MODULE_DESCRIPTION("SHMAC project serial driver");
