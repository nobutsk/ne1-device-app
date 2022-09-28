


#ifndef LED_CTRL_H__
#define LED_CTRL_H__

#include "ne1_define.h"
#include "nrf_drv_pwm.h"

#define LED_OFF 0          
#define LED_ON  1          
#define LED_LOW_FREQ  2    //点滅周期　s
#define LED_HIGH_FREQ 3    //点滅周期　s
#define LED_DOUBLE    4    //2回点滅
#define LED_TRIPLE    5    //3回点滅
#define LED_PW_START  6

#define LED_RESERVE         0
#define LED_NONE            1
#define LED_POWER_ON        2
#define LED_RAW_SEND        3
#define LED_TASK_END        4
#define LED_ADVERTISE       5
#define LED_BATT_LOW        6
#define LED_PAIRING         7
#define LED_RESET_END       8
#define LED_CHG_FULL        9
#define LED_CHARGING        10
#define LED_FW_OAD_UPD      11
#define LED_ERROR           12


void led_pwm_init(void);
void led_pwm_uninit(void);
static void led_lighting(led_color_t led);
void led_deviceStatusParsing(dev_status_t dev);
void led_toggle(uint8_t flag,led_color_t led);
uint8_t led_checkLedState(void);
static void gradually_handler(nrf_drv_pwm_evt_type_t event_type);


#endif