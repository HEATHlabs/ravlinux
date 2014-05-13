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

#define DRIVER_NAME "shmac-uart"
#define DEV_NAME "ttyshmc"
#define SERIAL_SHMAC_MAJOR 204
#define SERIAL_SHMAC_MINOR 214

#define UARTn_STATUS 0x20
#define UARTn_STATUS_TXBUSY 0x2
#define UARTn_RXDATA 0x10
#define UARTn_TXDATA 0x0

struct shmac_uart_port {
	struct uart_port port;
};

#define to_shmac_port(_port) container_of(_port, struct shmac_uart_port, port)

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

static void shmac_uart_stop_rx(struct uart_port *port)
{
	/* Stop receiving characters, the port is in the process of being closed */
}

static void shmac_console_putchar(struct uart_port *port, int ch);

static void shmac_tx_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;

        if (port->x_char) {
                /* send port->x_char out the port here */
                shmac_console_putchar(port, port->x_char);
                port->icount.tx++;
                port->x_char = 0;
                return;
        }
 
	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		shmac_uart_stop_tx(port);
		return;
	}
        
        while (1)
                if(!uart_circ_empty(xmit) && !uart_tx_stopped(port)) {
                        port->icount.tx++;
                        shmac_console_putchar(port, xmit->buf[xmit->tail]);
                        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
                } else
                        break;

        if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);
        
	if (uart_circ_empty(xmit))
		shmac_uart_stop_tx(port);
}

static void shmac_uart_start_tx(struct uart_port *port)
{
        /* Start transmitting characters */
        shmac_tx_chars(port);
}


static void shmac_uart_enable_ms(struct uart_port *port)
{
	/* Enable the modem status interrupts */
}

static void shmac_uart_break_ctl(struct uart_port *port, int ctl)
{
	/* Control the transission of a break signal */
}

static irqreturn_t shmac_uart_rxirq(int irq, void *data){
        struct shmac_uart_port *shmac_port = data;
        struct uart_port *port = &shmac_port->port;
        struct tty_port *tport = &port->state->port;
        char rxchar;
        spin_lock(&port->lock);
        
        rxchar = readl_relaxed(port->membase + UARTn_RXDATA);
        port->icount.rx++;
        tty_insert_flip_char(tport, rxchar, 0);

        spin_unlock(&port->lock);
        tty_flip_buffer_push(tport);
        return IRQ_HANDLED;
}

static int shmac_uart_startup(struct uart_port *port)
{
	struct shmac_uart_port *shmac_port = to_shmac_port(port);
        int ret;
        
        ret = request_irq(port->irq, shmac_uart_rxirq, 0, DRIVER_NAME, shmac_port);
        if(ret){
                pr_err("failed to register rxirq\n");
                return ret;
        }
	return 0;
}

static void shmac_uart_shutdown(struct uart_port *port)
{
       struct shmac_uart_port *shmac_port = to_shmac_port(port);
       
       free_irq(port->irq, shmac_port);
}

static void shmac_uart_set_termios(struct uart_port *port, struct ktermios *termios, struct ktermios *old)
{
        // Nothing really matters
}

static const char * shmac_uart_type(struct uart_port *port)
{
	return port->type == PORT_SHMACUART ? "shmac-uart" : NULL;
}

static void shmac_uart_release_port(struct uart_port *port)
{
        iounmap(port->membase);
}

static int shmac_uart_request_port(struct uart_port *port)
{
        struct shmac_uart_port *shmac_port = to_shmac_port(port);
       
        port->membase = ioremap(port->mapbase, 60);
	if (!shmac_port->port.membase) {
                pr_warn("failed to map memory\n");
		return -ENOMEM;
	}
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

// SHMAC has support for 16 UARTs
static struct shmac_uart_port *shmac_uart_ports[16];

#ifdef CONFIG_SERIAL_SHMAC_UART_CONSOLE
static void shmac_console_putchar(struct uart_port *port, int ch)
{
        while(readl_relaxed(port->membase + UARTn_STATUS) & UARTn_STATUS_TXBUSY);
        writel_relaxed(ch, port->membase + UARTn_TXDATA);
}

static void shmac_uart_console_write(struct console *co, const char *s, unsigned int count)
{
	struct shmac_uart_port *shmac_port = shmac_uart_ports[co->index];
        uart_console_write(&shmac_port->port, s, count, shmac_console_putchar);
}

static int shmac_uart_console_setup(struct console *co, char *options)
{
        struct shmac_uart_port *shmac_port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
        
        if (co->index < 0 || co->index >= ARRAY_SIZE(shmac_uart_ports)) {
		unsigned i;
		for (i = 0; i < ARRAY_SIZE(shmac_uart_ports); ++i) {
			if (shmac_uart_ports[i]) {
				pr_warn("shmac-console: fall back to console index %u (from %hhi)\n",
						i, co->index);
				co->index = i;
				break;
			}
		}
	}
        
        shmac_port = shmac_uart_ports[co->index];
        if(!shmac_port){
                pr_warn("shmac-console: No port at %d\n", co->index);
                return -ENODEV;
        }
        return uart_set_options(&shmac_port->port, co, baud, parity, bits, flow);
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
        .major = SERIAL_SHMAC_MAJOR,
        .minor = SERIAL_SHMAC_MINOR,
	.nr = ARRAY_SIZE(shmac_uart_ports),
	.cons = &shmac_uart_console,
};

static int shmac_uart_probe_dt(struct platform_device *pdev, struct shmac_uart_port *shmac_port)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;

	if(!np)
		return 1;
     
	ret = of_alias_get_id(np, "serial");
       
	if(ret < 0){
		dev_err(&pdev->dev, "failed to get alias id: %d\n", ret);
		return ret;
	} else {
                shmac_port->port.line = ret;
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
	
	ret = platform_get_irq(pdev,0);
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
                kfree(shmac_port);
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
	ret = uart_register_driver(&shmac_uart_reg);
	if (ret){
		return ret;
        }
	ret = platform_driver_register(&shmac_uart_driver);
	if (ret){
		uart_unregister_driver(&shmac_uart_reg);
        }
        pr_info("SHMAC serial init done\n");
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
