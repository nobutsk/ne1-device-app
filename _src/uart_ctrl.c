#include "ble_nus.h"
#include "app_uart.h"
#include "app_scheduler.h"
#include "ble_hrs.h"
#include "nrf52832_peripherals.h"

#include "ecg_define.h"
#include "ecg_main.h"
#include "gpio_ctrl.h"
#include "uart_ctrl.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#define UART_TX_BUF_SIZE                1024//256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                1024//256                                         /**< UART RX buffer size. */

/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' '\n' (hex 0x0A) or if the string has reached the maximum data length.
 */
/**@snippet [Handling the data received over UART] */


void uart_command_parse(app_uart_evt_t * p_event){
    static uint8_t data_buf;
    static uint8_t pkt_buf[30];
    static uint8_t pkt_each_byte=0;
    static uint8_t pkt_index=0;
    static uint8_t plength=0;
    static uint8_t plen_count=0;
    static uint8_t pkt_capture_done=0;

    static uint8_t ble_pkt[BLE_NUS_MAX_DATA_LEN];
    static uint8_t raw_sample_index=0;
    static uint8_t signal_quality=0;
    static uint8_t ble_pkt_send_flag=0;
    static uint8_t event_save[1000]={};
    static uint32_t event_count=0;
    if(event_count<1000){
      event_save[event_count]=p_event->evt_type;
      event_count++;
    }

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            break;
        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            return;
        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            return;
        default:
            return;
    }

    UNUSED_VARIABLE(app_uart_get(&data_buf));

    //1st. capture packet
    switch (pkt_each_byte){
        case 0:
            if(data_buf==BMD_SYNC)pkt_each_byte++;
        return;
        case 1:
            if(data_buf==BMD_SYNC)pkt_each_byte++;
            else pkt_each_byte=0;
        return;
        case 2:
            plength=data_buf;
            pkt_each_byte++;
        return;
        case 3:
            pkt_buf[plen_count]=data_buf;
            plen_count++;
            if(plen_count>plength){
                plength=0;
                plen_count=0;
                pkt_each_byte=0;
                pkt_capture_done=1;
                //NRF_LOG_INFO("pkt=%x_%x_%x_%x",pkt_buf[0],pkt_buf[1],pkt_buf[2],pkt_buf[3]);
            }          
        break;  
        default:
            pkt_each_byte=0;
        break; 
    }     

    //2nd. create packet
    if(pkt_capture_done==1){
        pkt_capture_done=0;
        if(pkt_buf[0]==BMD_CODE_RAW){
            ble_pkt[PKT_HEAD+raw_sample_index*2]  =pkt_buf[2];
            ble_pkt[PKT_HEAD+raw_sample_index*2+1]=pkt_buf[3];
            //NRF_LOG_INFO("pkt=%x_%x",ble_pkt[PKT_HEAD+raw_sample_index*2],ble_pkt[PKT_HEAD+raw_sample_index*2+1]);
            raw_sample_index++;
        }else if(pkt_buf[0]==BMD_CODE_SQ){
            signal_quality=pkt_buf[1];
            //NRF_LOG_INFO("SQ=%d",signal_quality);
        }else {
        }
        if(raw_sample_index>=RAW_PKT_LIMIT){
            raw_sample_index=0;
            ble_pkt[0]=pkt_index;
            ble_pkt[1]=signal_quality;
            if(pkt_index>=PKT_INDEX_LIMIT)pkt_index=0;
            pkt_index++;
            ble_pkt_send_flag=1;
            //NRF_LOG_INFO("----SEND----");
        }
    }
    //3rd. send packet
    if(ble_pkt_send_flag==1){
        ble_pkt_send_flag=0;
        uint16_t length = PKT_HEAD+RAW_PKT_LIMIT*2;
        ble_rawdata_send(ble_pkt,length);

        //do
        //{
        //    uint16_t length = PKT_HEAD+RAW_PKT_LIMIT*2;
        //    err_code = ble_hrs_heart_rate_measurement_send(&m_hrs,ble_pkt,&length);//, &length, m_conn_handle);// ble_nus_data_send(&m_nus, ble_pkt, &length, m_conn_handle);
        //    if ((err_code != NRF_ERROR_INVALID_STATE) &&
        //        (err_code != NRF_ERROR_RESOURCES) &&
        //        (err_code != NRF_ERROR_NOT_FOUND))
        //    {
        //        APP_ERROR_CHECK(err_code);
        //    }
        //} while (err_code == NRF_ERROR_RESOURCES);
        memset(&ble_pkt,0,BLE_NUS_MAX_DATA_LEN);
    }
}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
void uart_init(uint8_t txRxConfig)
{
    uint32_t                     err_code;
    app_uart_comm_params_t comm_params =
    {
        .rx_pin_no    = PIN_BMD_TX,
        .tx_pin_no    = PIN_BMD_RX_2,
        .rts_pin_no   = UART_PIN_DISCONNECTED,
        .cts_pin_no   = UART_PIN_DISCONNECTED,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
#if defined (UART_PRESENT)
        .baud_rate    = NRF_UART_BAUDRATE_57600//NRF_UART_BAUDRATE_115200
#else
        .baud_rate    = NRF_UARTE_BAUDRATE_57600
#endif
    };
    if(txRxConfig==UART_TX_ONLY){
      comm_params.rx_pin_no=UART_PIN_DISCONNECTED;
    }

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOW_MID,//APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    APP_ERROR_CHECK(err_code);
}
////////////////////////////////////////
         //event handler
////////////////////////////////////////


void uart_event_handle(app_uart_evt_t * p_event)
{
    app_sched_event_put(p_event, sizeof(p_event), uart_scheduler_event_handler);
}




//old parse func
//void uart_command_parse(app_uart_evt_t * p_event){
//    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
//    static uint8_t index = 0;

//    static uint8_t data_buf;
//    static uint8_t pkt_buf[30];
//    static uint8_t pkt_each_byte=0;
//    static uint8_t pkt_index=0;
//    static uint8_t plength=0;
//    static uint8_t plen_count=0;
//    static uint8_t pkt_capture_done=0;

//    static uint8_t ble_pkt[BLE_NUS_MAX_DATA_LEN];
//    static uint8_t raw_sample_index=0;
//    static uint8_t signal_quality=0;
//    static uint8_t ble_pkt_send_flag=0;
//    static uint8_t event_save[1000]={};
//    static uint32_t event_count=0;
//    if(event_count<1000){
//      event_save[event_count]=p_event->evt_type;
//      event_count++;
//    }

//    switch (p_event->evt_type)
//    {
//        case APP_UART_DATA_READY:
//            UNUSED_VARIABLE(app_uart_get(&data_buf));

//            //1st. capture packet
//            if(pkt_each_byte==0){
//                if(data_buf==BMD_SYNC)pkt_each_byte++;
//            }else if(pkt_each_byte==1){
//                if(data_buf==BMD_SYNC)pkt_each_byte++;
//                else pkt_each_byte=0;
//            }else if(pkt_each_byte==2){
//                plength=data_buf;
//                pkt_each_byte++;
//            }else if(pkt_each_byte==3){
//                pkt_buf[plen_count]=data_buf;
//                plen_count++;
//                if(plen_count>plength){
//                    plength=0;
//                    plen_count=0;
//                    pkt_each_byte=0;
//                    pkt_capture_done=1;
//                    //NRF_LOG_INFO("pkt=%x_%x_%x_%x",pkt_buf[0],pkt_buf[1],pkt_buf[2],pkt_buf[3]);
//                }
//            }else {
//                pkt_each_byte=0;
//            }
//            //2nd. create packet
//            if(pkt_capture_done==1){
//                pkt_capture_done=0;
//                if(pkt_buf[0]==BMD_CODE_RAW){
//                    ble_pkt[PKT_HEAD+raw_sample_index*2]  =pkt_buf[2];
//                    ble_pkt[PKT_HEAD+raw_sample_index*2+1]=pkt_buf[3];
//                    //NRF_LOG_INFO("pkt=%x_%x",ble_pkt[PKT_HEAD+raw_sample_index*2],ble_pkt[PKT_HEAD+raw_sample_index*2+1]);

//                    raw_sample_index++;
//                }else if(pkt_buf[0]==BMD_CODE_SQ){
//                    signal_quality=pkt_buf[1];
//                    //NRF_LOG_INFO("SQ=%d",signal_quality);
//                }else {
//                }
//                if(raw_sample_index>=RAW_PKT_LIMIT){
//                    raw_sample_index=0;
//                    ble_pkt[0]=pkt_index;
//                    ble_pkt[1]=signal_quality;
//                    if(pkt_index>=PKT_INDEX_LIMIT)pkt_index=0;
//                    pkt_index++;
//                    ble_pkt_send_flag=1;
//                    //NRF_LOG_INFO("----SEND----");
//                }
//            }
//            //3rd. send packet
//            if(ble_pkt_send_flag==1){
//                ble_pkt_send_flag=0;
//                uint16_t length = PKT_HEAD+RAW_PKT_LIMIT*2;
//                ble_rawdata_send(ble_pkt,length);

//                //do
//                //{
//                //    uint16_t length = PKT_HEAD+RAW_PKT_LIMIT*2;
//                //    err_code = ble_hrs_heart_rate_measurement_send(&m_hrs,ble_pkt,&length);//, &length, m_conn_handle);// ble_nus_data_send(&m_nus, ble_pkt, &length, m_conn_handle);
//                //    if ((err_code != NRF_ERROR_INVALID_STATE) &&
//                //        (err_code != NRF_ERROR_RESOURCES) &&
//                //        (err_code != NRF_ERROR_NOT_FOUND))
//                //    {
//                //        APP_ERROR_CHECK(err_code);
//                //    }
//                //} while (err_code == NRF_ERROR_RESOURCES);
//                memset(&ble_pkt,0,BLE_NUS_MAX_DATA_LEN);
//            }
//            break;

//        case APP_UART_COMMUNICATION_ERROR:
//            APP_ERROR_HANDLER(p_event->data.error_communication);
//            break;

//        case APP_UART_FIFO_ERROR:
//            APP_ERROR_HANDLER(p_event->data.error_code);
//            break;

//        default:
//            break;
//    }
//}