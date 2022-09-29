#include <stdint.h>
#include <string.h>
#include "sdk_config.h"
#include "nordic_common.h"
#include "nrf.h"

#include "nrf_sdm.h"
#include "common.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_hrs.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "app_timer.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "fds.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_lesc.h"
#include "nrf_ble_qwr.h"
#include "ble_conn_state.h"
#include "nrf_pwr_mgmt.h"
#include "app_scheduler.h"
#include "nrf_drv_gpiote.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"

//add library
#include "nrf_delay.h"

//USER CREATED
#include "ne1_define.h"
#include "ne1_main.h"
#include "gpio_ctrl.h"
#include "led_ctrl.h"
#include "twi_ctrl.h"

#define DEAD_BEEF                           0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

BLE_HRS_DEF(m_hrs);                                                 /**< Heart rate service instance. */
BLE_BAS_DEF(m_bas);                                                 /**< Structure used to identify the battery service. */
NRF_BLE_GATT_DEF(m_gatt);                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                 /**< Advertising module instance. */

APP_TIMER_DEF(m_battery_timer_id);                                  /**< Battery timer. */
APP_TIMER_DEF(m_periodic_task_id);                               /**< Heart rate measurement timer. */
APP_TIMER_DEF(m_led_timer_id);                              /**< RR interval timer. */
APP_TIMER_DEF(m_power_sw_id);                           /**< Sensor contact detected timer. */


dev_status_t dev;
dev_status_t prevDev;

static uint16_t m_conn_handle         = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */
static bool     m_rr_interval_enabled = true;                       /**< Flag for enabling and disabling the registration of new RR interval measurements (the purpose of disabling this is just to test sending HRM without RR interval data. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static bool adv_flag=0;

static uint16_t battery_level=100;
static uint8_t batt_save_low_lev=100;
static uint8_t power_on=1;

static bool power_off_pend=0;
static bool led_disp_processing=0;
static bool erase_config_flag=0;
static bool pm_evt_peer_delete_flag=1;
static bool ble_connect_flag=0;
static bool batt_lev_upd_ble_send_enable=0;

uint8_t bme_meas_state=0;

//static ble_uuid_t m_adv_uuids[] =                                   /**< Universally unique service identifiers. */
//{
//    {BLE_UUID_HEART_RATE_SERVICE,           BLE_UUID_TYPE_BLE},
//    {BLE_UUID_BATTERY_SERVICE,              BLE_UUID_TYPE_BLE},
//    {BLE_UUID_DEVICE_INFORMATION_SERVICE,   BLE_UUID_TYPE_BLE}
//};

uint32_t disable_ble(void);
static void main_second_init(void);

void device_status_ctrl(uint8_t stat,uint8_t batt,uint8_t flag){
//    prevDev.status =dev.status;
//    prevDev.battery=dev.battery;
//    prevDev.flag   =dev.flag;
    if(stat!=0)dev.status   =stat;
    if(batt!=0)dev.battery  =batt;
    if(flag!=0)dev.flag     =flag;
    led_deviceStatusParsing(dev);
    NRF_LOG_INFO("dev=%d,%d,%d", dev.status,dev.battery,dev.flag);

}

void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}
/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

static void power_off_process(void){
    NRF_LOG_INFO("power off start");
    disable_ble();

    if(dev.status!=DEV_STAT_POWER_OFF)device_status_ctrl(DEV_STAT_POWER_OFF,DEV_BATT_NORMAL,DEV_FLAG_NONE);
    app_timer_stop(m_battery_timer_id);
    app_timer_stop(m_periodic_task_id);
    app_timer_stop(m_led_timer_id);
    app_timer_stop(m_power_sw_id);

    if(erase_config_flag){
        delete_bonds();
        //while(pm_evt_peer_delete_flag);
    }

    //nrf_delay_ms(10);
    //app_timer_stop(m_battery_timer_id);
    //app_timer_stop(m_periodic_task_id);
    ret_code_t err_code;
    err_code = nrf_sdh_disable_request();
    APP_ERROR_CHECK(err_code);
    led_pwm_uninit();

    gpio_button_config_enable();
    power_on=0;
}



/**@brief Function for starting advertising.
 */
void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    }
    else
    {
        ret_code_t err_code;

        err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
        APP_ERROR_CHECK(err_code);
        if(adv_flag)device_status_ctrl(DEV_STAT_ADVERTISE,DEV_NO_CHG,DEV_NO_CHG);
    }
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_disconnect_on_sec_failure(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            //advertising_start(false); bond erase power_off_process
            pm_evt_peer_delete_flag=0;
            break;

        default:
            break;
    }
}

void ble_rawdata_send(uint8_t *blePkt,uint16_t len){
    uint32_t       err_code;
    do
    {
        err_code = ble_hrs_heart_rate_measurement_send(&m_hrs,blePkt,&len);//, &length, m_conn_handle);// ble_nus_data_send(&m_nus, blePkt, &length, m_conn_handle);
        if ((err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != NRF_ERROR_RESOURCES) &&
            (err_code != NRF_ERROR_NOT_FOUND))
        {
            APP_ERROR_CHECK(err_code);
        }
    } while (err_code == NRF_ERROR_RESOURCES);
}

/**@brief Function for performing battery measurement and updating the Battery Level characteristic
 *        in Battery Service.
 */
static void battery_level_update(void)
{
    ret_code_t err_code;

    battery_level =twi_battery_State_Of_Charge_get();
    if(DBG_BATT_LOG)NRF_LOG_INFO("batt_result=%d" , battery_level);
    if(batt_lev_upd_ble_send_enable){
        if(DBG_BATT_LOG)NRF_LOG_INFO("send-ble-battery=%d", battery_level);

        err_code = ble_bas_battery_level_update(&m_bas, battery_level, BLE_CONN_HANDLE_ALL);
        if ((err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != NRF_ERROR_RESOURCES) &&
            (err_code != NRF_ERROR_BUSY) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
           )
        {
            APP_ERROR_HANDLER(err_code);
        }
    }
}


/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    battery_level_update();
    //NRF_LOG_INFO("battLevelUpdate");
}


static void periodic_task(void){
    static uint8_t usb_on_flag=0;
    static uint16_t auto_power_off_count=0;
    static uint8_t init_count=0;
    static uint8_t * ble_pkt_8;

    if(!nrf_gpio_pin_read(PIN_5V_LEV)){//usb eject
         //NRF_LOG_INFO(" usb eject");
        if(ble_connect_flag&&bme_meas_state){
            ble_pkt_8=app_bme68x_meas();
            //memcpy(ble_pkt_8,(uint8_t *)ble_pkt_32,3);
            //printf("%x,%x,%x,%x\n",
            //ble_pkt_8[0],ble_pkt_8[1],ble_pkt_8[2],ble_pkt_8[3]);

            ble_rawdata_send(ble_pkt_8,20);
        }
        if(init_count==0){
            init_count++;
            battery_level_update();
        }

        if(dev.status!=DEV_STAT_POWER_OFF){ //eject usb
            if(usb_on_flag){
                NRF_LOG_INFO("power off : usb eject");
                device_status_ctrl(DEV_STAT_POWER_OFF,DEV_BATT_NORMAL,DEV_FLAG_PROCESS_END);
                power_off_process();                
            }
        }
        if(battery_level==OFF_BATTERY_LEVEL){ 
            //NRF_LOG_INFO("power off : battery empty");
            //device_status_ctrl(DEV_STAT_POWER_OFF,DEV_BATT_NORMAL,DEV_FLAG_NONE);
            //power_off_process();
            
        }else if(battery_level<=LOW_BATTERY_LEVEL){
            if(dev.battery!=DEV_BATT_LOW){
                device_status_ctrl(DEV_NO_CHG,DEV_BATT_LOW,DEV_NO_CHG);
                if(DBG_BATT_LOG)NRF_LOG_INFO("battery is low");
            }
        }else if(LOW_BATTERY_LEVEL<battery_level){
            if(dev.battery!=DEV_BATT_NORMAL){
                device_status_ctrl(DEV_NO_CHG,DEV_BATT_NORMAL,DEV_NO_CHG);
                if(DBG_BATT_LOG)NRF_LOG_INFO("battery is normal");
            }
        }
        ////autopoweroff
        //if( dev.status==DEV_STAT_PAIRING || dev.status==DEV_STAT_ADVERTISE){
        //    auto_power_off_count++;
        //    //NRF_LOG_INFO("auto_power_off_count=%d",auto_power_off_count);
        //    if(auto_power_off_count==0)NRF_LOG_INFO("auto power off timer start");
        //    if(auto_power_off_count>=AUTOPOWEROFF_TIME){
        //        device_status_ctrl(DEV_STAT_POWER_OFF,DEV_BATT_NORMAL,DEV_NO_CHG);
        //        NRF_LOG_INFO("power off : auto power off time up");
        //        power_off_process();
        //    }
        //}else {
        //    auto_power_off_count=0;
        //}
    }else {//usb insert
     //NRF_LOG_INFO(" usb insert");
        usb_on_flag=1;
        if(dev.status==DEV_STAT_RAWDATA_SEND){
            device_status_ctrl(DEV_STAT_PAIRING,DEV_NO_CHG,DEV_NO_CHG);
            NRF_LOG_INFO("rawdata send cancel : usb insert");
        }
        if(dev.battery!=DEV_BATT_CHARGE_FULL && battery_level==100){
            device_status_ctrl(DEV_NO_CHG,DEV_BATT_CHARGE_FULL,DEV_NO_CHG);
            if(DBG_BATT_LOG)NRF_LOG_INFO("battery is full");
        }
        if(dev.battery!=DEV_BATT_CHARGING    &&  battery_level<100){
            device_status_ctrl(DEV_NO_CHG,DEV_BATT_CHARGING,DEV_NO_CHG);
            disable_ble();
            if(DBG_BATT_LOG)NRF_LOG_INFO("battery charge");
        }
    }
}

static void periodic_task_timeout_handler(void * p_context)
{
    periodic_task();
}

static uint8_t led_blink_end=0;
static uint8_t led_blink_count=0;
static uint8_t previous_led_state=0;

void ledAppTimerStart(led_color_t led){
    ret_code_t err_code;
    static led_color_t led_stat; 
    led_stat=led;
    led_blink_count=0;
    switch (led.light_disp){
        case LED_LOW_FREQ:
            err_code = app_timer_start(m_led_timer_id, LED_TASK_INTERVAL_LOW, (led_color_t*)&led_stat);
        break;
        case LED_HIGH_FREQ:
            err_code = app_timer_start(m_led_timer_id, LED_TASK_INTERVAL_HIGH, (led_color_t*)&led_stat);
        break;
        case LED_PW_START:
            err_code = app_timer_start(m_led_timer_id, LED_TASK_INTERVAL_POWER_ON, (led_color_t*)&led_stat);
        break;
        case LED_DOUBLE:
        case LED_TRIPLE:
            err_code = app_timer_start(m_led_timer_id, LED_TASK_INTERVAL_MID, (led_color_t*)&led_stat);
            previous_led_state=led_checkLedState();
            //NRF_LOG_INFO("ledstate=%d",previous_led_state);
            led_blink_end=0;
        break;
        default:
        break;
    }  
    APP_ERROR_CHECK(err_code);
}
void ledAppTimerStop(void){
    app_timer_stop(m_led_timer_id);
//    if(DBG_LED_STAT)NRF_LOG_INFO("led app timer stop");
}

/**@brief Function for handling the RR interval timer timeout.
 *
 * @details This function will be called each time the RR interval timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void led_task_timeout_handler(void * p_context)
{
    led_color_t led;
    led=*((led_color_t*)p_context);     
    led_blink_count++;
    switch (led.light_disp){
        case LED_LOW_FREQ:
        case LED_HIGH_FREQ:
            if(led_blink_count%2){
                led_toggle(LED_ON,led);
            }else {
                led_toggle(LED_OFF,led);    
            }
            if(led_blink_count>1)led_blink_count=0;
        break;
        case LED_PW_START:
            device_status_ctrl(DEV_STAT_ADVERTISE,DEV_NO_CHG,DEV_NO_CHG);
            NRF_LOG_INFO("power on done");
        break; 
        case LED_DOUBLE:
        case LED_TRIPLE:
            if(previous_led_state==1){//if previous led is ON then led.flag blink start OFF state for smooth blink
                if(led_blink_count%2){
                    led_toggle(LED_OFF,led);
                }else {
                    led_toggle(LED_ON,led);
                }        
            }else if(previous_led_state==0){
                if(led_blink_count%2){
                    led_toggle(LED_ON,led);
                }else {
                    led_toggle(LED_OFF,led);
                }        
            }
            if(led.light_disp==LED_DOUBLE && led_blink_count>LED_DOUBLE_BLINK_LIM)led_blink_end=1;
            if(led.light_disp==LED_TRIPLE && led_blink_count>LED_TRIPLE_BLINK_LIM)led_blink_end=1;
            if(led_blink_end){
                led_blink_count=0;
                led_blink_end=0;
                led_disp_processing=0;

                device_status_ctrl(DEV_NO_CHG,DEV_NO_CHG,DEV_FLAG_NONE);
                if(power_off_pend||led.light_disp==LED_TRIPLE)power_off_process();    

                
            }
        break;
     
        default:
        break;
    }  
}


/**@brief Function for handling the Sensor Contact Detected timer timeout.
 *
 * @details This function will be called each time the Sensor Contact Detected timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static uint16_t psw_long_push_count=0;
static uint8_t  psw_state=0;
static uint8_t  power_off_flag=0;
static void power_sw_timeout_handler(void * p_context)
{
    //    gpioLedSet();
    psw_long_push_count++;
    psw_state= nrf_gpio_pin_read(PIN_SW);
    //nrf_gpio_port_in_read
    if(DBG_PSW_STAT)NRF_LOG_INFO("pswCount:%d,State:%d,power_off_flag:%d",psw_long_push_count,psw_state,power_off_flag);
    if(!psw_state){//push release
        if(psw_long_push_count<PW_OFF_SW_LONG_PUSH_TIME){
            //no operation(miss push)
            NRF_LOG_INFO("No ope.");
        }else {
            //power off
            NRF_LOG_INFO("powerOff : power sw long push");
            power_off_flag=1; 
            power_off_pend=1;
            if(!led_disp_processing)power_off_process();
            gpio_button_config_disable();
        }
        psw_state=0;
        psw_long_push_count=0;
        app_timer_stop(m_power_sw_id);
    }else if(psw_state){//push hold
        if(psw_long_push_count<PW_OFF_SW_LONG_PUSH_TIME){
            //NRF_LOG_INFO("No ope.");
        }else if(psw_long_push_count<PW_OFF_SW_LONG_PUSH_CFG_RST){
            if(power_off_flag==0){
                if(DBG_PSW_STAT)NRF_LOG_INFO("powerOff : power sw long push");
                device_status_ctrl(DEV_STAT_POWER_OFF,DEV_NO_CHG,DEV_FLAG_PROCESS_END);
                led_disp_processing=1;              
            }
            power_off_flag=1;      
            //      psw_long_push_count=0;
            //power off
        }else if(PW_OFF_SW_LONG_PUSH_CFG_RST<psw_long_push_count){
            if(power_off_flag==1){
                //erase bonding & power off
                if(DBG_PSW_STAT)NRF_LOG_INFO("powerOff : power sw more long push + erase bonding");
                device_status_ctrl(DEV_NO_CHG,DEV_NO_CHG,DEV_FLAG_RESET_END);
                led_disp_processing=1;
                erase_config_flag=1;
            }
            power_off_flag=2;
        }
    }
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    ret_code_t err_code;

    // Initialize timer module.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create timers.
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&m_periodic_task_id,
                                APP_TIMER_MODE_REPEATED,
                                periodic_task_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_led_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                led_task_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_power_sw_id,
                                APP_TIMER_MODE_REPEATED,
                                power_sw_timeout_handler);
    APP_ERROR_CHECK(err_code);
}
/**@brief Function for starting application timers.
 */
static void application_timers_start(void)
{
    ret_code_t err_code;

    // Start application timers.
    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_periodic_task_id, PERIODIC_TASK_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

//POWER SW EVNT
static void button_handler(nrf_drv_gpiote_pin_t pin)
{
    ret_code_t err_code;
    power_off_flag=0;
    psw_long_push_count=0;
    psw_state=0;

    if(!power_on){
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_RESET);
        //bool erase_bonds=0;
        //advertising_start(erase_bonds);
        //application_timers_start();
        //power_on=1;
        //NRF_LOG_INFO("power_on=%d",power_on);
    }else {
        err_code = app_timer_start(m_power_sw_id, POWER_SW_TASK_INTERVAL, NULL);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Button handler function to be called by the scheduler.
 */
void button_scheduler_event_handler(void *p_event_data, uint16_t event_size)
{
    // In this case, p_event_data is a pointer to a nrf_drv_gpiote_pin_t that represents
    // the pin number of the button pressed. The size is constant, so it is ignored.
    
    button_handler(*((nrf_drv_gpiote_pin_t*)p_event_data));

}
void sense_5v_scheduler_event_handler(void *p_event_data, uint16_t event_size)
{
    // In this case, p_event_data is a pointer to a nrf_drv_gpiote_pin_t that represents
    // the pin number of the button pressed. The size is constant, so it is ignored.
    if(!power_on){
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_RESET);
    }else {
        gpio_button_config_disable();
        periodic_task();
    }
}
/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    //err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT);
    //APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);

    //ble_opt_t static_pin_option;
    //static uint8_t static_passkey[] = STATIC_PASSKEY;
    //static_pin_option.gap_opt.passkey.p_passkey = &static_passkey[0];
    //err_code = sd_ble_opt_set(BLE_GAP_OPT_PASSKEY, &static_pin_option);
    //APP_ERROR_CHECK(err_code);
}


/**@brief GATT module event handler.
 */
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED)
    {
        NRF_LOG_INFO("GATT ATT MTU on connection 0x%x changed to %d.",
                     p_evt->conn_handle,
                     p_evt->params.att_mtu_effective);
    }

    ble_hrs_on_gatt_evt(&m_hrs, p_evt);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the Heart Rate, Battery and Device Information services.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    ble_hrs_init_t     hrs_init;
    ble_bas_init_t     bas_init;
    ble_dis_init_t     dis_init;
    nrf_ble_qwr_init_t qwr_init = {0};
    uint8_t            body_sensor_location;

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Battery Service.
    memset(&bas_init, 0, sizeof(bas_init));

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    // Here the sec level for the Battery Service can be changed/increased.
    bas_init.bl_rd_sec        = SEC_OPEN;
    bas_init.bl_cccd_wr_sec   = SEC_OPEN;
    bas_init.bl_report_rd_sec = SEC_OPEN;

    err_code = ble_bas_init(&m_bas, &bas_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Device Information Service.
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);
    ble_srv_ascii_to_utf8(&dis_init.serial_num_str, (char *)SERIAL_NUMBER);
    ble_srv_ascii_to_utf8(&dis_init.model_num_str, (char *)MODEL_NUMBER);
    ble_srv_ascii_to_utf8(&dis_init.hw_rev_str, (char *)HW_REVISION);
    ble_srv_ascii_to_utf8(&dis_init.fw_rev_str, (char *)FW_REVISION);
    ble_srv_ascii_to_utf8(&dis_init.sw_rev_str, (char *)SW_REVISION);

    dis_init.dis_char_rd_sec = SEC_OPEN;

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Heart Rate Service.
    memset(&hrs_init, 0, sizeof(hrs_init));

    hrs_init.evt_handler                 = NULL;
    hrs_init.is_sensor_contact_supported = true;

    // Here the sec level for the Heart Rate Service can be changed/increased.
    hrs_init.hrm_cccd_wr_sec = SEC_OPEN;
    hrs_init.hs_wr_sec      = SEC_OPEN;

    err_code = ble_hrs_init(&m_hrs, &hrs_init);
    APP_ERROR_CHECK(err_code);

}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;//m_hrs.hrm_handles.cccd_handle;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

bool run_adv = true;

uint32_t disable_ble(void)
{
    uint32_t err_code;
    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
        run_adv = false;
    }
    else
    {
        err_code = sd_ble_gap_adv_stop(m_advertising.adv_handle);
    }
    return err_code;
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
            if (run_adv == false)
            {
                sd_ble_gap_adv_stop(m_advertising.adv_handle);
            }
            //err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            //APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            //sleep_mode_enter();
            break;

        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");
            //err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            //APP_ERROR_CHECK(err_code);
            ble_connect_flag=1;
            device_status_ctrl(DEV_STAT_PAIRING,DEV_NO_CHG,DEV_NO_CHG);
            batt_lev_upd_ble_send_enable=1;
            
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            err_code = pm_conn_secure(p_ble_evt->evt.gap_evt.conn_handle, false);
            if (err_code != NRF_ERROR_BUSY)
            {
                APP_ERROR_CHECK(err_code);
            }
            NRF_LOG_INFO("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_1MBPS,//BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_1MBPS,//BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            device_status_ctrl(DEV_STAT_ADVERTISE,DEV_NO_CHG,DEV_NO_CHG);
            ble_connect_flag=0;
            bme_meas_state=0;
            batt_lev_upd_ble_send_enable=0;
            NRF_LOG_INFO("Disconnected, reason %d.",
                          p_ble_evt->evt.gap_evt.params.disconnected.reason);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_INFO("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_1MBPS,//BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_1MBPS,//BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
/////////Removed for ebonding process//////////
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            NRF_LOG_DEBUG("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
            break;
        
        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_KEY_REQUEST");
            break;

        case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_LESC_DHKEY_REQUEST");
            break;

         case BLE_GAP_EVT_AUTH_STATUS:
             NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lesc: %d kdist_own:0x%x kdist_peer:0x%x",
                          p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
                          p_ble_evt->evt.gap_evt.params.auth_status.bonded,
                          p_ble_evt->evt.gap_evt.params.auth_status.lesc,
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer));
             NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS 2 :lv1-4:%d,%d,%d,%d",                          
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv1,
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv2,
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv3,
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4);
             NRF_LOG_FLUSH();
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

}

/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;
    


    memset(&init, 0, sizeof(init));

    ble_uuid128_t const uuid_base   = BLE_BASE_CUS_UUID;
    uint8_t uuid_type_ven           = BLE_UUID_TYPE_VENDOR_BEGIN;
    err_code = sd_ble_uuid_vs_add(&uuid_base,&uuid_type_ven);
    static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_CUS_SERVICE,BLE_UUID_TYPE_VENDOR_BEGIN}};

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

    //init.config.ble_adv_primary_phy      = BLE_GAP_PHY_2MBPS;
    //init.config.ble_adv_secondary_phy    = BLE_GAP_PHY_2MBPS;
    //init.config.ble_adv_extended_enabled = true;

    //init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    //init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}
/**@brief Function for the Event Scheduler initialization.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    app_sched_execute();

    ret_code_t err_code;  
    err_code = nrf_ble_lesc_request_handler();
    APP_ERROR_CHECK(err_code);

    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}
static void main_second_init(void){
    //battery_level_update();
    //bool erase_bonds=0;
    //advertising_start(erase_bonds);
    //adv_flag=1;
    //gpio_button_config_init();
}




void power_mgmt_test(){

    // Initialize.
    log_init();
    gpio_init();

    power_management_init();
    scheduler_init();  
    //ret_code_t err;
    //err=nrf_sdh_disable_request();
    //APP_ERROR_CHECK(err);
    nrf_pwr_mgmt_run();

    //// Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }

    while(1);
}

int main(void)
{
    //power_mgmt_test();
    // Initialize.
    log_init();
    gpio_init();

    NRF_LOG_INFO("gpio init");
    NRF_LOG_FLUSH();

    twi_init();
    app_bme68x_init();
   
    timers_init();
    power_management_init();
    ble_stack_init();
    scheduler_init();   
    gap_params_init();
    gatt_init();
    advertising_init();
    services_init();
    conn_params_init();
    peer_manager_init();

    NRF_LOG_INFO("ne1 start.");
    application_timers_start();

    bool erase_bonds=0;
    advertising_start(erase_bonds);

    led_pwm_init();

    adv_flag=1;
    gpio_button_config_init();

    if(!nrf_gpio_pin_read(PIN_5V_LEV)){
        device_status_ctrl(DEV_STAT_POWER_ON,DEV_BATT_NORMAL,DEV_FLAG_NONE);
    }
    else {
        device_status_ctrl(DEV_NO_CHG,DEV_BATT_CHARGING,DEV_NO_CHG);
        periodic_task();
    }

    //// Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}


