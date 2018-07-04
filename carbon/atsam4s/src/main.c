#include "libflipper.h"
#include "atsam4s.h"
#include "os/scheduler.h"
#include "gpio.h"

/* How many clock cycles to wait before giving up initialization. */
#define CLOCK_TIMEOUT 5000

struct _fmr_packet packet;
static struct _lf_device *_4s;

extern void uart0_put(uint8_t byte);

int debug_putchar(char c, FILE *stream) {
	uart0_put(c);
	return 0;
}

void os_kernel_task(void) {
	// gpio_enable(IO_1, 0);
	// while (1) {
	// 	printf("Hello!\n");
	// 	gpio_write(IO_1, 0);
	// 	for (int i = 0x1FFFFFC; i > 0; i --) __asm__ __volatile__ ("nop");
	// 	gpio_write(0, IO_1);
	// 	for (int i = 0x1FFFFFC; i > 0; i --) __asm__ __volatile__ ("nop");
	// }
}

/* Use the uart0 bus as the read/write endpoint. */

int atsam4s_read(struct _lf_device *device, void *dst, uint32_t length) {
	return uart0_read(dst, length);
}

int atsam4s_write(struct _lf_device *device, void *src, uint32_t length) {
	return uart0_write(src, length);
}

int atsam4s_release(struct _lf_device *device) {
	return lf_error;
}

int main(void) {

	/* Disable the watchdog timer. */
	WDT->WDT_MR = WDT_MR_WDDIS;

	/* Configure the EFC for 5 wait states. */
	EFC0->EEFC_FMR = EEFC_FMR_FWS(PLATFORM_WAIT_STATES);

	/* Configure the primary clock source. */
	if (!(PMC->CKGR_MOR & CKGR_MOR_MOSCSEL)) {
		PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD | BOARD_OSCOUNT | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTEN;
		for (uint32_t timeout = 0; !(PMC->PMC_SR & PMC_SR_MOSCXTS) && (timeout ++ < CLOCK_TIMEOUT););
	}

	/* Select external 20MHz oscillator. */
	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD | BOARD_OSCOUNT | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTEN | CKGR_MOR_MOSCSEL;
	for (uint32_t timeout = 0; !(PMC->PMC_SR & PMC_SR_MOSCSELS) && (timeout ++ < CLOCK_TIMEOUT););
	PMC->PMC_MCKR = (PMC->PMC_MCKR & ~(uint32_t)PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
	for (uint32_t timeout = 0; !(PMC->PMC_SR & PMC_SR_MCKRDY) && (timeout++ < CLOCK_TIMEOUT););

	/* Configure PLLB as the master clock PLL. */
	PMC->CKGR_PLLBR = BOARD_PLLBR;
	for (uint32_t timeout = 0; !(PMC->PMC_SR & PMC_SR_LOCKB) && (timeout++ < CLOCK_TIMEOUT););

	/* Switch to the main clock. */
	PMC->PMC_MCKR = (BOARD_MCKR & ~PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
	for (uint32_t timeout = 0; !(PMC->PMC_SR & PMC_SR_MCKRDY) && (timeout++ < CLOCK_TIMEOUT););
	PMC->PMC_MCKR = BOARD_MCKR;
	for (uint32_t timeout = 0; !(PMC->PMC_SR & PMC_SR_MCKRDY) && (timeout++ < CLOCK_TIMEOUT););

	/* Allow the reset pin to reset the device. */
	RSTC->RSTC_MR = RSTC_MR_KEY_PASSWD | RSTC_MR_URSTEN;

	_4s = lf_device_create(atsam4s_read, atsam4s_write, atsam4s_release);
	lf_attach(_4s);

#if 0

    /* peripheral configuration */

    extern struct _lf_module adc;
	dyld_register(_4s, &adc);
    adc_configure();

    extern struct _lf_module button;
	dyld_register(_4s, &button);
    button_configure();

    extern struct _lf_module dac;
	dyld_register(_4s, &dac);
    dac_configure();

    extern struct _lf_module gpio;
	dyld_register(_4s, &gpio);
    gpio_configure();

    extern struct _lf_module i2c;
	dyld_register(_4s, &i2c);
    i2c_configure();

    extern struct _lf_module led;
	dyld_register(_4s, &led);
    led_configure();

    extern struct _lf_module pwm;
	dyld_register(_4s, &pwm);
    pwm_configure();

    extern struct _lf_module rtc;
	dyld_register(_4s, &rtc);
    rtc_configure();

    extern struct _lf_module spi;
	dyld_register(_4s, &spi);
    spi_configure();

    extern struct _lf_module swd;
	dyld_register(_4s, &swd);
    swd_configure();

    extern struct _lf_module temp;
	dyld_register(_4s, &temp);
    temp_configure();

    extern struct _lf_module timer;
	dyld_register(_4s, &timer);
    timer_configure();

    extern struct _lf_module uart0;
	dyld_register(_4s, &uart0);
    uart0_configure();

    extern struct _lf_module usart;
	dyld_register(_4s, &usart);
    usart_configure();

    extern struct _lf_module usb;
	dyld_register(_4s, &usb);
    usb_configure();

    extern struct _lf_module wdt;
	dyld_register(_4s, &wdt);
	wdt_configure();

#endif

	/* Enable the FSI pin. */
	gpio_enable(FMR_PIN, 0);
	gpio_write(0, FMR_PIN);

	/* Pull an FMR packet asynchronously to launch FMR. */
	uart0_read(&packet, sizeof(struct _fmr_packet));
	/* Enable the PDC receive complete interrupt. */
	UART0->UART_IER = UART_IER_ENDRX;

	/* Launch the kernel task. */
	os_scheduler_init();
}

void uart0_isr(void) {

	__disable_irq();

	uint32_t _sr = UART0->UART_SR;

	/* If an entire packet has been received, process it. */
	if (_sr & UART_SR_ENDRX) {
		/* set fmr low (active) */
		gpio_write(0, FMR_PIN);

		UART0->UART_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;

        lf_error_set(E_OK);
		fmr_perform(_4s, &packet);

		/* Pull an FMR packet asynchronously to launch FMR. */
		uart0_read(&packet, sizeof(struct _fmr_packet));

		/* Wait a bit before raising the FMR pin. */
		for (size_t i = 0; i < 0x3FF; i ++) __asm__ __volatile__("nop");

		/* set fmr high (inactive) */
		gpio_write(FMR_PIN, 0);
	} else {
		UART0->UART_CR = UART_CR_RSTSTA;
	}

	__enable_irq();

}

void uart0_pull_wait(void *dst, uint32_t length) {
	/* Disable the PDC receive complete interrupt. */
	UART0->UART_IDR = UART_IDR_ENDRX;
	/* Set the transmission length and destination pointer. */
	UART0->UART_RCR = length;
	UART0->UART_RPR = (uintptr_t)dst;
	/* Enable the receiver. */
	UART0->UART_PTCR = UART_PTCR_RXTEN;
	/* Wait until the transfer has finished. */
	while (!(UART0->UART_SR & UART_SR_ENDRX));
	/* Disable the PDC receiver. */
	UART0->UART_PTCR = UART_PTCR_RXTDIS;
	/* Enable the PDC receive complete interrupt. */
	UART0->UART_IER = UART_IER_ENDRX;
}
