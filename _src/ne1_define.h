
#ifndef ECG_DEF_H__
#define ECG_DEF_H__

#define DBG_BATT_LOG 0
#define DBG_PSW_STAT 0
#define DBG_LED_STAT 0

//advertisement
#define DEVICE_NAME                         "ne1"                            /**< Name of device. Will be included in the advertising data. */
//device information service
#define MANUFACTURER_NAME                   "NI-denshi"                             /**< Manufacturer. Will be passed to Device Information Service. */
#define MODEL_NUMBER                        "ne1"                             /**< Manufacturer. Will be passed to Device Information Service. */
#define SERIAL_NUMBER                       "ne1"                             /**< Manufacturer. Will be passed to Device Information Service. */
#define HW_REVISION                         "HW.0.1"                             /**< Manufacturer. Will be passed to Device Information Service. */
#define FW_REVISION                         "FW.0.1"                             /**< Manufacturer. Will be passed to Device Information Service. */
#define SW_REVISION                         "SW.0.1"                             /**< Manufacturer. Will be passed to Device Information Service. */


#define BLE_BASE_CUS_UUID            {{0x20, 0x1e, 0x45, 0x3b, 0x4c, 0xc7 ,  0x58, 0xf1, 0xab, 0x5f ,0x6a, 0xe4 ,0xBB, 0xAA, 0x05, 0x4b}} /**< Used vendor specific UUID. */

#define BLE_UUID_CUS_SERVICE      0xAABB               /**< The UUID of the TX Characteristic. */
#define BLE_UUID_CUS_NOTIFY       0xAAA0               /**< The UUID of the TX Characteristic. */
#define BLE_UUID_CUS_WRITE        0xAAA1               /**< The UUID of the RX Characteristic. */

#define APP_ADV_INTERVAL                    300                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */
#define APP_ADV_DURATION                    18000                                   /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define APP_BLE_CONN_CFG_TAG                1                                       /**< A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_OBSERVER_PRIO               3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */


#define BATT_LEV_UPP_LIM 900
#define BATT_LEV_LOW_LIM 688
#define BATT_ADC_SMPL_PERIOD_MS 5000
#define BATT_LEV_SEND_PERIOD_MS 5000//1000
#define BATTERY_LEVEL_MEAS_INTERVAL         APP_TIMER_TICKS(BATT_LEV_SEND_PERIOD_MS)//2000)                   /**< Battery level measurement interval (ticks). */

#define PERIODIC_TASK_INTERVAL              APP_TIMER_TICKS(2000)//1000                  

#define LED_TASK_INTERVAL_LOW               APP_TIMER_TICKS(1000)                   /**< Heart rate measurement interval (ticks). */
#define LED_TASK_INTERVAL_MID               APP_TIMER_TICKS(200)                   /**< Heart rate measurement interval (ticks). */
#define LED_TASK_INTERVAL_HIGH              APP_TIMER_TICKS(100)                   /**< Heart rate measurement interval (ticks). */
#define LED_TASK_INTERVAL_POWER_ON          APP_TIMER_TICKS(1500)
#define LED_DOUBLE_BLINK_LIM                4
#define LED_TRIPLE_BLINK_LIM                6


#define POWER_SW_TASK_INTERVAL              APP_TIMER_TICKS(100)                   /**< Heart rate measurement interval (ticks). */

#define PW_OFF_SW_LONG_PUSH_TIME            10//１秒            /* Power Off 長押し時間                  */
#define PW_OFF_SW_LONG_PUSH_CFG_RST         80//8秒            /* Bonding削除 長押し時間      */
#define PW_OFF_SW_LONG_PUSH_TIME_LIMIT      100
#define MIN_CONN_INTERVAL                   MSEC_TO_UNITS(15, UNIT_1_25_MS)//100        /**< Minimum acceptable connection interval (0.4 seconds). */
#define MAX_CONN_INTERVAL                   MSEC_TO_UNITS(30, UNIT_1_25_MS)//650        /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                       0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                    MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */


#define AUTOPOWEROFF_TIME                   300   //unit:[seconds]
    
#define LOW_BATTERY_LEVEL                   15          //LOWバッテリー検出レベル[%]（残量上下限値補正後）
#define OFF_BATTERY_LEVEL                   0

#define FIRST_CONN_PARAMS_UPDATE_DELAY      APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY       APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT        3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define LESC_DEBUG_MODE                     0                                       /**< Set to 1 to use LESC debug keys, allows you to use a sniffer to inspect traffic. */

#define SEC_PARAM_BOND                      1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                      0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                      1                                       /**< LE Secure Connections enabled. */
#define SEC_PARAM_KEYPRESS                  0                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES           BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
//#define SEC_PARAM_IO_CAPABILITIES           BLE_GAP_IO_CAPS_DISPLAY_ONLY                    /**< No I/O capabilities. */

#define SEC_PARAM_OOB                       0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE              7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE              16                                      /**< Maximum encryption key size. */
#define STATIC_PASSKEY    "123456"

#define SCHED_MAX_EVENT_DATA_SIZE       APP_TIMER_SCHED_EVENT_DATA_SIZE             /**< Maximum size of scheduler events. */
#ifdef SVCALL_AS_NORMAL_FUNCTION
#define SCHED_QUEUE_SIZE                20                                          /**< Maximum number of events in the scheduler queue. More is needed in case of Serialization. */
#else
#define SCHED_QUEUE_SIZE                20                                          /**< Maximum number of events in the scheduler queue. */
#endif


//parser
#define BMD_SYNC        0xAA
#define BMD_CODE_SQ     0x02
#define BMD_CODE_HR     0x03
#define BMD_CODE_RAW    0x80

#define BMD_SQ_ON       200
#define BMD_SQ_OFF      0

#define BMD_PLEN_RAW    0x04
#define BMD_PLEN_SQ_HR  0x12

#define RAW_PKT_LIMIT 9 //amount of raw sample(not Bytes) for packet create  
#define PKT_INDEX_LIMIT 30
#define PKT_HEAD 2
//Handshake decoding
#define BME_MODE         0

#define BME_MEAS_START   0x01
#define BME_MEAS_STOP    0x02

#define HS_REG_CHG      0x3C

#define HS_NO_CHG       0
//command decoding
#define BMD_CMD_LEN     7


#define UART_TX_RX   0
#define UART_TX_ONLY 1

#define DEV_NO_CHG                  0 // Keep current value 

#define DEV_STAT_POWER_OFF          1
#define DEV_STAT_POWER_ON           2
#define DEV_STAT_ADVERTISE          3
#define DEV_STAT_PAIRING            4
#define DEV_STAT_RAWDATA_SEND       5
#define DEV_STAT_BULK_REG_UPD       6
#define DEV_STAT_SINGLE_REG_UPD     7
#define DEV_STAT_DEVICE_INFO_READ   8
#define DEV_STAT_FW_OAD_UPD         9
#define DEV_STAT_ERROR              10


#define DEV_BATT_NORMAL             1
#define DEV_BATT_LOW                2
#define DEV_BATT_CHARGING           3
#define DEV_BATT_CHARGE_FULL        4

#define DEV_FLAG_NONE               1
#define DEV_FLAG_PROCESS_END        2
#define DEV_FLAG_RESET_END          3

typedef struct {
    uint8_t led_mode;
    uint8_t light_disp;
    uint16_t ring;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} led_color_t;

typedef struct {
    uint8_t status;
    uint8_t battery;
    uint8_t flag;
} dev_status_t;



extern uint32_t g_data;
extern uint8_t bme_meas_state;

#endif