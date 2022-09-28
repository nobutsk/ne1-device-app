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

#include "app_scheduler.h"

#include "ecg_define.h"
#include "ecg_main.h"
#include "adc_ctrl.h"


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"



volatile uint8_t state = 1;
static const nrf_drv_timer_t m_timer = NRF_DRV_TIMER_INSTANCE(1);

static nrf_saadc_value_t     m_buffer_pool[2][SAMPLES_IN_BUFFER];
static nrf_ppi_channel_t     m_ppi_channel;
static uint32_t              m_adc_evt_counter;

uint16_t saadc_result[SAMPLES_IN_BUFFER];

void timer_handler(nrf_timer_event_t event_type, void * p_context)
{

}


void saadc_sampling_event_init(void)
{
    ret_code_t err_code;

    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    err_code = nrf_drv_timer_init(&m_timer, &timer_cfg, timer_handler);
    APP_ERROR_CHECK(err_code);

    /* setup m_timer for compare event every 1s 400ms */
    uint32_t ticks = nrf_drv_timer_ms_to_ticks(&m_timer, 100);
    nrf_drv_timer_extended_compare(&m_timer,
                                   NRF_TIMER_CC_CHANNEL1,
                                   ticks,
                                   NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK,
                                   false);
    nrf_drv_timer_enable(&m_timer);

    uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(&m_timer,
                                                                                NRF_TIMER_CC_CHANNEL1);
    uint32_t saadc_sample_task_addr   = nrf_drv_saadc_sample_task_get();

    /* setup ppi channel so that timer compare event is triggering sample task in SAADC */
    err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(m_ppi_channel,
                                          timer_compare_event_addr,
                                          saadc_sample_task_addr);
    APP_ERROR_CHECK(err_code);
}

static void saadc_sampling_event_timer_update(void){
    nrf_drv_timer_disable(&m_timer);
    /* setup m_timer for compare event every 1s 400ms */
    uint32_t ticks = nrf_drv_timer_ms_to_ticks(&m_timer, BATT_ADC_SMPL_PERIOD_MS);
    nrf_drv_timer_extended_compare(&m_timer,
                                   NRF_TIMER_CC_CHANNEL1,
                                   ticks,
                                   NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK,
                                   false);
    nrf_drv_timer_enable(&m_timer);
}

void saadc_sampling_event_enable(void)
{
    ret_code_t err_code = nrf_drv_ppi_channel_enable(m_ppi_channel);

    APP_ERROR_CHECK(err_code);
}

void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        ret_code_t err_code;

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
        APP_ERROR_CHECK(err_code);

        int i;
        //NRF_LOG_INFO("ADC event number: %d", (int)m_adc_evt_counter);

        for (i = 0; i < SAMPLES_IN_BUFFER; i++)
        {
            //NRF_LOG_INFO("%d", p_event->data.done.p_buffer[i]);
            saadc_result[i]=p_event->data.done.p_buffer[i];
        }
        //NRF_LOG_HEXDUMP_INFO(saadc_result,5);
        m_adc_evt_counter++;
        if(m_adc_evt_counter)saadc_sampling_event_timer_update();
    }
}


void saadc_init(void)
{
    ret_code_t err_code;
    for(uint8_t i=0;i<SAMPLES_IN_BUFFER;i++)saadc_result[i]=0;

    nrf_saadc_channel_config_t channel_config =
    NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);//pin 0.02

    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);
    ///NRF_LOG_HEXDUMP_INFO(m_buffer_pool[0],5);
    //if use double buffer
    //err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], SAMPLES_IN_BUFFER);
    //APP_ERROR_CHECK(err_code);nrfx_saadc_buffer_convert
    
    saadc_sampling_event_init();
    saadc_sampling_event_enable();

    nrf_drv_saadc_sample_convert(0,m_buffer_pool[0]);

    nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
    NRF_LOG_HEXDUMP_INFO(m_buffer_pool[0],5);
}
void saadc_uninit(void){
    ret_code_t err_code;
    nrf_drv_saadc_abort();
    err_code=nrf_drv_saadc_channel_uninit(0);
    APP_ERROR_CHECK(err_code);
    nrf_drv_saadc_uninit();
    //saadc_sampling_event_uninit
    err_code=nrf_drv_ppi_uninit();
    APP_ERROR_CHECK(err_code);
    nrf_drv_timer_uninit(&m_timer);

}

uint16_t saadc_result_convert_to_batt_disp(void)
{
    uint16_t battery_lev_calc=0;

    if(saadc_result[0]>BATT_LEV_UPP_LIM)battery_lev_calc=100;
    else if(saadc_result[0]<BATT_LEV_LOW_LIM)battery_lev_calc=0;
    else {
        battery_lev_calc=saadc_result[0]*0.467;//BATT_LEV_UPP_LIM;
        battery_lev_calc=battery_lev_calc-321;
    }

    return battery_lev_calc;
}
