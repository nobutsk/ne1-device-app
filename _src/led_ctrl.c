/*
File    F led_ctrl.c
Details : led initialize ,change
Author  : Iitsuka
 */

#include "nrf_pwm.h"
#include "nrf_drv_pwm.h"
#include "nrfx.h"
#include "nrf_egu.h"
#include "nrfx_log.h"

#include "gpio_ctrl.h"
#include "led_ctrl.h"
#include "ne1_define.h"
#include "ne1_main.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define L_OFF 0
#define L_LOW 300
#define L_MID 3000
#define L_MAX 10000
#define L_RNG_OFF 10000
//#define L_RNG_OFF 0

const led_color_t LED_TYPE[]=
{
 //MAX(period)=3000
 //led_mode     ,light_disp   ,RING       ,RED        ,GREEN      ,BLUE   
 {LED_RESERVE   ,LED_OFF       ,L_RNG_OFF  ,L_OFF      ,L_OFF      ,L_OFF  },       
 {LED_NONE      ,LED_OFF       ,L_RNG_OFF  ,L_OFF      ,L_OFF      ,L_OFF  },       
 {LED_POWER_ON  ,LED_PW_START  ,L_MID      ,L_OFF      ,L_MID      ,L_OFF  },       
 {LED_RAW_SEND  ,LED_LOW_FREQ  ,L_MID      ,L_OFF      ,L_MID      ,L_OFF  },
 {LED_TASK_END  ,LED_DOUBLE    ,L_MID      ,L_OFF      ,L_MID      ,L_OFF  },
 {LED_ADVERTISE ,LED_ON        ,L_MID      ,L_MID      ,L_LOW      ,L_OFF  },
 {LED_BATT_LOW  ,LED_LOW_FREQ  ,L_MID      ,L_MID      ,L_OFF      ,L_OFF  },
 //{LED_BATT_LOW  ,LED_ON  ,L_MID      ,L_MID      ,L_OFF      ,L_OFF  },
 {LED_PAIRING   ,LED_ON        ,L_MID      ,L_OFF      ,L_MID      ,L_OFF  },
 {LED_RESET_END ,LED_TRIPLE    ,L_RNG_OFF  ,L_MID      ,L_LOW      ,L_OFF  },
 {LED_CHG_FULL  ,LED_ON        ,L_RNG_OFF  ,L_OFF      ,L_OFF      ,L_MID  },
 {LED_CHARGING  ,LED_LOW_FREQ  ,L_RNG_OFF  ,L_OFF      ,L_OFF      ,L_MID  },
 {LED_FW_OAD_UPD,LED_HIGH_FREQ ,L_MID      ,L_OFF      ,L_OFF      ,L_OFF  },
 {LED_ERROR     ,LED_ON        ,L_MID      ,L_MID      ,L_OFF      ,L_OFF  }
};

static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);
static nrf_drv_pwm_t m_pwm1 = NRF_DRV_PWM_INSTANCE(1);
static nrf_drv_pwm_t m_pwm2 = NRF_DRV_PWM_INSTANCE(2);

static uint8_t m_used = 0;

static uint16_t const              ledPwmTopValue  = L_MAX;
static uint16_t const              ledPwmStepValue = 150;
static uint8_t                     ledPhase;
static nrf_pwm_values_individual_t ledPwm;
static nrf_pwm_sequence_t const    ledPwmSeq =
{
    .values.p_individual = &ledPwm,
    .length              = NRF_PWM_VALUES_LENGTH(ledPwm),
    .repeats             = 0,
    .end_delay           = 0
};



void led_pwm_init(void)
{
    nrf_drv_pwm_config_t const config0 =
    {
        .output_pins =
        {
            PIN_LED_SUB  ,//| NRF_DRV_PWM_PIN_INVERTED, // channel 0
            PIN_LED_RED   | NRF_DRV_PWM_PIN_INVERTED, // channel 1
            PIN_LED_GREEN | NRF_DRV_PWM_PIN_INVERTED, // channel 2
            PIN_LED_BLUE  | NRF_DRV_PWM_PIN_INVERTED  // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_1MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = ledPwmTopValue,
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode    = NRF_PWM_STEP_AUTO
    };
    APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm0, &config0, NULL));
    //APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm0, &config0, gradually_handler));
//    m_used |= USED_PWM(0);

    ledPwm.channel_0 = L_RNG_OFF;
    ledPwm.channel_1 = L_OFF;
    ledPwm.channel_2 = L_OFF;
    ledPwm.channel_3 = L_OFF;
    ledPhase           = 0;

    (void)nrf_drv_pwm_simple_playback(&m_pwm0, &ledPwmSeq, 1,
                                      NRF_DRV_PWM_FLAG_LOOP);
}
void led_pwm_uninit(void){
    nrf_drv_pwm_uninit(&m_pwm0);
}

void led_deviceStatusParsing(dev_status_t dev){
    ////Error 
    //if(dev.status==DEV_STAT_ERROR){
    //    led_lighting(LED_TYPE[LED_ERROR]);
    //    return;
    //}
    //First Priority
    switch (dev.flag){
      case  DEV_FLAG_NONE://No operation
      break;
      case  DEV_FLAG_PROCESS_END:
        led_lighting(LED_TYPE[LED_TASK_END]);
        return;
      break;
      case  DEV_FLAG_RESET_END:
        led_lighting(LED_TYPE[LED_RESET_END]);
        return;
      break;
      default :
      //led_lighting(LED_TYPE[LED_NONE]);
      break;
    }
    //Second Priority
    //&& DEV_FLAG_NONE
    switch (dev.battery){
      case  DEV_BATT_NORMAL://No operation
      break;
      case  DEV_BATT_LOW:
        led_lighting(LED_TYPE[LED_BATT_LOW]);
        return;
      break;
      case DEV_BATT_CHARGING:
        if(dev.status==DEV_STAT_FW_OAD_UPD){
            led_lighting(LED_TYPE[LED_FW_OAD_UPD]);
        }else {
            led_lighting(LED_TYPE[LED_CHARGING]);
        }
        return;
      break;
      case DEV_BATT_CHARGE_FULL:
        led_lighting(LED_TYPE[LED_CHG_FULL]);
        return;
      break;
      default :
      //led_lighting(LED_TYPE[LED_NONE]);
      break;
    }
    //Third Priority
    // && DEV_FLAG_NONE && DEV_BATT_NORMAL
    switch (dev.status){
      case  DEV_STAT_POWER_OFF:
        led_lighting(LED_TYPE[LED_NONE]);
      break;
      case  DEV_STAT_POWER_ON:
        led_lighting(LED_TYPE[LED_POWER_ON]);
      break;
      case  DEV_STAT_ADVERTISE:
        led_lighting(LED_TYPE[LED_ADVERTISE]);
                //NRF_LOG_INFO("led.%d,%d,%d,%d,%d",led.light_disp,led.ring,led.red,led.green,led.blue);
      break;
      case  DEV_STAT_PAIRING:
        led_lighting(LED_TYPE[LED_PAIRING]);
                //NRF_LOG_INFO("led.%d,%d,%d,%d,%d",led.light_disp,led.ring,led.red,led.green,led.blue);
      break;
      case  DEV_STAT_RAWDATA_SEND:
        led_lighting(LED_TYPE[LED_RAW_SEND]);
      break;
      default :
        led_lighting(LED_TYPE[LED_NONE]);
      break;
    }
}

void led_lighting(led_color_t led){
    ledAppTimerStop();
    switch (led.light_disp){
      case LED_OFF:
        ledPwm.channel_0 = L_RNG_OFF;
        ledPwm.channel_1 = L_OFF;
        ledPwm.channel_2 = L_OFF;
        ledPwm.channel_3 = L_OFF;
      break;
      case LED_ON:
        ledPwm.channel_0 = led.ring;
        ledPwm.channel_1 = led.red;
        ledPwm.channel_2 = led.green;
        ledPwm.channel_3 = led.blue;
        NRF_LOG_INFO("led=mode_%d,disp_%d,ring_%d,r_%d,g_%d,b_%d",led.led_mode,led.light_disp,led.ring,led.red,led.green,led.blue);
      break;
      case LED_PW_START:
        ledPwm.channel_0 = led.ring;
        ledPwm.channel_1 = led.red;
        ledPwm.channel_2 = led.green;
        ledPwm.channel_3 = led.blue;
        NRF_LOG_INFO("led=mode_%d,disp_%d,ring_%d,r_%d,g_%d,b_%d",led.led_mode,led.light_disp,led.ring,led.red,led.green,led.blue);
        // Start applicaction timer
        ledAppTimerStart(led);
        break;
      case LED_LOW_FREQ:
      case LED_HIGH_FREQ:
      case LED_DOUBLE:
      case LED_TRIPLE:
        // Start applicaction timer
        ledAppTimerStart(led);
      break;
      default:
      break;
    }
}
void led_toggle(uint8_t flag,led_color_t led){
    if(flag){
        ledPwm.channel_0 = led.ring;
        ledPwm.channel_1 = led.red;
        ledPwm.channel_2 = led.green;
        ledPwm.channel_3 = led.blue;      
    }else {
        //ledPwm.channel_0 = L_RNG_OFF;
        ledPwm.channel_1 = L_OFF;
        ledPwm.channel_2 = L_OFF;
        ledPwm.channel_3 = L_OFF; 
    }
}
uint8_t led_checkLedState(void){
    //if( ledPwm.channel_0 == L_RNG_OFF&&
    if( ledPwm.channel_1 == L_OFF&&
        ledPwm.channel_2 == L_OFF&&
        ledPwm.channel_3 == L_OFF)return LED_OFF;
    return LED_ON;
}


//led gradually blink (test)
static void gradually_handler(nrf_drv_pwm_evt_type_t event_type)
{
    if (event_type == NRF_DRV_PWM_EVT_FINISHED)
    {
        uint8_t channel    = ledPhase >> 1;
        bool    down       = ledPhase & 1;
        bool    next_phase = false;

        if(channel==0){
            uint16_t * p_channels = (uint16_t *)&ledPwm;
            uint16_t value = p_channels[channel];
            if (down)
            {
                value -= ledPwmStepValue;
                if (value == 0)
                {
                    next_phase = true;
                }
            }
            else
            {
                value += ledPwmStepValue;
                if (value >= ledPwmTopValue)
                {
                    next_phase = true;
                }
            }
            p_channels[channel] = value;

            if (next_phase)
            {
                if (++ledPhase >= 2 * NRF_PWM_CHANNEL_COUNT)
                {
                    ledPhase = 0;
                }
            }
        }
    }
}
