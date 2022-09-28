#ifndef ADC_CTRL_H__
#define ADC_CTRL_H__

#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"


#define SAMPLES_IN_BUFFER 1

void timer_handler(nrf_timer_event_t event_type, void * p_context);
void saadc_sampling_event_init(void);
void saadc_sampling_event_enable(void);
void saadc_callback(nrf_drv_saadc_evt_t const * p_event);
void saadc_init(void);
void saadc_uninit(void);
uint16_t saadc_result_convert_to_batt_disp(void);

#endif