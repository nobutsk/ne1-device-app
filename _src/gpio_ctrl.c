#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrfx_ppi.h"

#include "boards.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "app_util_platform.h"

#include "nrf_gpio.h"
#include "gpio_ctrl.h"
#include "app_scheduler.h"
#include "nrf_drv_gpiote.h"

#include "ne1_define.h"
#include "ne1_main.h"


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


////////////////////////////////////////
         //Initialize
////////////////////////////////////////

void gpio_init(void){

// config for LED
  nrf_gpio_cfg(
    PIN_LED_RED,
    NRF_GPIO_PIN_DIR_OUTPUT,
    NRF_GPIO_PIN_INPUT_DISCONNECT,
    NRF_GPIO_PIN_NOPULL,
    NRF_GPIO_PIN_H0H1,  //select Standard O High1
    NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_pin_set(PIN_LED_RED); 
  nrf_gpio_cfg(
    PIN_LED_GREEN,
    NRF_GPIO_PIN_DIR_OUTPUT,
    NRF_GPIO_PIN_INPUT_DISCONNECT,
    NRF_GPIO_PIN_NOPULL,
    NRF_GPIO_PIN_H0H1,  //select Standard O High1
    NRF_GPIO_PIN_NOSENSE); 
  nrf_gpio_pin_set(PIN_LED_GREEN);
  nrf_gpio_cfg(
    PIN_LED_BLUE,
    NRF_GPIO_PIN_DIR_OUTPUT,
    NRF_GPIO_PIN_INPUT_DISCONNECT,
    NRF_GPIO_PIN_NOPULL,
    NRF_GPIO_PIN_H0H1,  //select Standard O High1
    NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_pin_set(PIN_LED_BLUE);

  nrf_gpio_cfg_input(PIN_STAT,NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(PIN_SW,NRF_GPIO_PIN_PULLDOWN);   

//  nrf_gpio_cfg_input(PIN_5V_LEV_2,NRF_GPIO_PIN_PULLUP);
//  nrf_gpio_cfg_input(PIN_5V_LEV,NRF_GPIO_PIN_PULLUP);

  nrf_gpio_cfg_input(PIN_5V_LEV,NRF_GPIO_PIN_NOPULL);

}

void gpio_button_config_init(void){ 
    ret_code_t err_code;

    // Initialze driver.
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    // Make a configuration for input pins. This is suitable for both pins in this example.
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    in_config.pull = NRF_GPIO_PIN_PULLDOWN;
    
    // Configure input pins for buttons, with separate event handlers for each button.
    err_code = nrf_drv_gpiote_in_init(PIN_SW, &in_config, gpiote_event_handler);
    APP_ERROR_CHECK(err_code);

    in_config.pull = NRF_GPIO_PIN_NOPULL;
    // Configure input pins for buttons, with separate event handlers for each button.
    err_code = nrf_drv_gpiote_in_init(PIN_5V_LEV, &in_config, gpiote_event_handler_5v);
    APP_ERROR_CHECK(err_code);

    // Enable input pins for buttons.
    nrf_drv_gpiote_in_event_enable(PIN_SW, true);
    nrf_drv_gpiote_in_event_enable(PIN_5V_LEV, true);
}
void gpio_button_config_disable(void){
    nrf_drv_gpiote_in_event_disable(PIN_SW);
}
void gpio_button_config_enable(void){
    nrf_drv_gpiote_in_event_enable(PIN_SW, true);
}



////////////////////////////////////////
         //event handler
////////////////////////////////////////


/**@brief Button event handler.
 */
static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // The button_handler function could be implemented here directly, but is extracted to a
    // separate function as it makes it easier to demonstrate the scheduler with less modifications
    // to the code later in the tutorial.

    app_sched_event_put(&pin, sizeof(pin), button_scheduler_event_handler);
}


/**@brief Button event handler.
 */
static void gpiote_event_handler_5v(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // The button_handler function could be implemented here directly, but is extracted to a
    // separate function as it makes it easier to demonstrate the scheduler with less modifications
    // to the code later in the tutorial.

    app_sched_event_put(&pin, sizeof(pin), sense_5v_scheduler_event_handler);
}



