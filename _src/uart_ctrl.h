#ifndef UART_CTRL_H__
#define UART_CTRL_H__
#include "app_uart.h"
void uart_command_parse(app_uart_evt_t * p_event);
void uart_init(uint8_t txRxConfig);
void uart_event_handle(app_uart_evt_t * p_event);
#endif
