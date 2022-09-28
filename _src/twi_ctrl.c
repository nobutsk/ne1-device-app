#include <stdio.h>
#include "boards.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "nrf_drv_twi.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "twi_ctrl.h"
#include "gpio_ctrl.h"
#include <nrf_delay.h>
#include "common.h"
/* TWI instance ID. */
#if TWI0_ENABLED
#define TWI_INSTANCE_ID     0
#elif TWI1_ENABLED
#define TWI_INSTANCE_ID     1
#endif


/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

uint8_t twi_tx_buffer[100]={};
uint8_t twi_rx_buffer[100]={};

/**
 * @brief TWI initialization.
 */
void twi_init(void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
       .scl                = PIN_SCL,
       .sda                = PIN_SDA,
       .frequency          = NRF_DRV_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);
    nrf_drv_twi_enable(&m_twi);
}

void twi_tx(uint8_t addr,uint8_t tx_len){
    ret_code_t err_code;
    err_code = nrf_drv_twi_tx(&m_twi, addr, twi_tx_buffer, tx_len,0);
    APP_ERROR_CHECK(err_code);

}
void twi_rx(uint8_t addr,uint8_t rx_len){
    ret_code_t err_code;
    err_code = nrf_drv_twi_rx(&m_twi, addr, twi_rx_buffer, rx_len);
    APP_ERROR_CHECK(err_code);    
}


#define TWI_ADDR_MAX17048G 0x36
#define TWI_MAX17048G_SOC  0x04
#define BATT_SOC_UPP_LIM 93

uint8_t twi_battery_State_Of_Charge_get(void){
    uint8_t battery_SOC=0;
    twi_tx_buffer[0] = TWI_MAX17048G_SOC;
    twi_tx(TWI_ADDR_MAX17048G,1);
    twi_rx(TWI_ADDR_MAX17048G,2);
    if(twi_rx_buffer[0]>BATT_SOC_UPP_LIM)battery_SOC=100;
    else if(twi_rx_buffer[0]==0)battery_SOC=0;
    else battery_SOC=twi_rx_buffer[0]/0.93;//BATT_SOC_UPP_LIM*0.01

    return battery_SOC;
}



int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
    twi_tx_buffer[0]=reg_addr;
    twi_tx(dev_id,1);
//    twi_rx_buffer[0]=reg_addr;
    twi_rx(dev_id,len);
    memcpy(reg_data,twi_rx_buffer,len);

    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Stop       | -                   |
     * | Start      | -                   |
     * | Read       | (reg_data[0])       |
     * | Read       | (....)              |
     * | Read       | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */

    return rslt;
}

int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
    twi_tx_buffer[0]=reg_addr;
    memcpy(&twi_tx_buffer[1],reg_data,len);
    twi_tx(dev_id,len+1);
    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Write      | (reg_data[0])       |
     * | Write      | (....)              |
     * | Write      | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */

    return rslt;
}

//void bme68x_test(void){
//    bme68x_interface_init();
//}

////void user_delay_ms(uint32_t period){
////    nrf_delay_ms(period);
////}

//struct bme680_dev gas_sensor;
//void bme68x_init(void){
//    gas_sensor.dev_id = BME680_I2C_ADDR_PRIMARY;
//    gas_sensor.intf = BME680_I2C_INTF;
//    gas_sensor.read = user_i2c_read;
//    gas_sensor.write = user_i2c_write;
//    gas_sensor.delay_ms = nrf_delay_ms;//user_delay_ms;
//    /* amb_temp can be set to 25 prior to configuring the gas sensor 
//     * or by performing a few temperature readings without operating the gas sensor.
//     */
//    gas_sensor.amb_temp = 25;


//    int8_t rslt = BME680_OK;
//    rslt = bme680_init(&gas_sensor);
//}

 /* Number of possible TWI addresses. */
#define TWI_ADDRESSES      127
/**
 * @brief Function for main application entry.
 */
void twi_test(void)
{
    ret_code_t err_code;
    uint8_t address;
    uint8_t sample_data;
    uint8_t ans;
    bool detected_device = false;

    //APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    //NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("TWI scanner started.");

    twi_init();
    NRF_LOG_INFO("TWI size of %d.",sizeof(sample_data));
    NRF_LOG_FLUSH();

    for (address = 1; address <= TWI_ADDRESSES; address++)
    {
        err_code = nrf_drv_twi_rx(&m_twi, address, &sample_data, sizeof(sample_data));
        if (err_code == NRF_SUCCESS)
        {
            detected_device = true;
            NRF_LOG_INFO("TWI device detected at address 0x%x.", address);
        }else {
        NRF_LOG_INFO("No device.");
        }
        NRF_LOG_FLUSH();
    }

    if (!detected_device)
    {
        NRF_LOG_INFO("No device was found.");
        NRF_LOG_FLUSH();
    }

    while (true)
    {
        /* Empty loop. */
    }
}
