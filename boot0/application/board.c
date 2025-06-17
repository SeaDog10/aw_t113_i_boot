#include "main.h"
#include "board.h"
#include "sunxi_gpio.h"
#include "sunxi_usart.h"
#include "sunxi_spi.h"

sunxi_usart_t usart0_dbg = {
	.base	 = 0x02500000,
	.id		 = 0,
	.gpio_tx = {GPIO_PIN(PORTG, 17), GPIO_PERIPH_MUX7},
	.gpio_rx = {GPIO_PIN(PORTG, 18), GPIO_PERIPH_MUX7},
};

sunxi_spi_t sunxi_spi0 = {
	.base	   = 0x04025000,
	.id		   = 0,
	.clk_rate  = 100 * 1000 * 1000,
	.gpio_cs   = {GPIO_PIN(PORTC, 3), GPIO_PERIPH_MUX2},
	.gpio_sck  = {GPIO_PIN(PORTC, 2), GPIO_PERIPH_MUX2},
	.gpio_mosi = {GPIO_PIN(PORTC, 4), GPIO_PERIPH_MUX2},
	.gpio_miso = {GPIO_PIN(PORTC, 5), GPIO_PERIPH_MUX2},
	.gpio_wp   = {GPIO_PIN(PORTC, 6), GPIO_PERIPH_MUX2},
	.gpio_hold = {GPIO_PIN(PORTC, 7), GPIO_PERIPH_MUX2},
};


static gpio_t led_blue = GPIO_PIN(PORTC, 0);

void board_init_led(gpio_t led)
{
	sunxi_gpio_init(led, GPIO_OUTPUT);
	sunxi_gpio_set_value(led, 0);
}

void board_init()
{
	board_init_led(led_blue);
	sunxi_usart_init(&USART_DBG);
}
