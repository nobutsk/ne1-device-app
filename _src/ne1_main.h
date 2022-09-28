#ifndef ECG_MAIN_H__
#define ECG_MAIN_H__


#include "ne1_define.h"
void button_scheduler_event_handler(void *p_event_data, uint16_t event_size);
void sense_5v_scheduler_event_handler(void *p_event_data, uint16_t event_size);
void device_status_ctrl(uint8_t stat,uint8_t batt,uint8_t flag);

void ble_rawdata_send(uint8_t *blePkt,uint16_t len);
void ledAppTimerStart(led_color_t led);
void ledAppTimerStop(void);
#endif
