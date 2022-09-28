#ifndef GPIO_CTRL_H__
#define GPIO_CTRL_H__

#include "nrf_drv_gpiote.h"

//ne1
#define PIN_SDA         0
#define PIN_SCL         1
#define PIN_5V_LEV      2
#define PIN_LED_SUB     3
#define PIN_LED_BLUE    5
#define PIN_LED_RED     6
#define PIN_LED_GREEN   8
#define PIN_SW          22
#define PIN_STAT        28
#define PIN_5V_LEV_2    30

void gpio_init(void);
void gpio_do(void);
void gpioLedSet(void);

static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
static void gpiote_event_handler_5v(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

void gpio_button_config_init(void);
void gpio_button_config_disable(void);
void gpio_button_config_enable(void);

#endif
