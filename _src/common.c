/**
 * Copyright (C) 2021 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "bme68x.h"
//#include "coines.h"
#include "common.h"
#include "twi_ctrl.h"
#include <nrf_delay.h>
/******************************************************************************/
/*!                 Macro definitions                                         */
/*! BME68X shuttle board ID */
#define BME68X_SHUTTLE_ID  0x93

/******************************************************************************/
/*!                Static variable definition                                 */
static uint8_t dev_addr;

/******************************************************************************/
/*!                User interface functions                                   */

/*!
 * I2C read function map to COINES platform
 */
BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t*)intf_ptr;

    return user_i2c_read(dev_addr, reg_addr, reg_data, (uint16_t)len);
}

/*!
 * I2C write function map to COINES platform
 */
BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t*)intf_ptr;

    return user_i2c_write(dev_addr, reg_addr, (uint8_t *)reg_data, (uint16_t)len);
}

///*!
// * SPI read function map to COINES platform
// */
//BME68X_INTF_RET_TYPE bme68x_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
//{
//    uint8_t dev_addr = *(uint8_t*)intf_ptr;

//    return coines_read_spi(dev_addr, reg_addr, reg_data, (uint16_t)len);
//}

///*!
// * SPI write function map to COINES platform
// */
//BME68X_INTF_RET_TYPE bme68x_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
//{
//    uint8_t dev_addr = *(uint8_t*)intf_ptr;

//    return coines_write_spi(dev_addr, reg_addr, (uint8_t *)reg_data, (uint16_t)len);
//}

/*!
 * Delay function map to COINES platform
 */
void bme68x_delay_us(uint32_t period, void *intf_ptr)
{
    nrf_delay_us(period);
}

void bme68x_check_rslt(const char api_name[], int8_t rslt)
{
    switch (rslt)
    {
        case BME68X_OK:
            //printf("API name [%s] done \r\n", api_name);
            /* Do nothing */
            break;
        case BME68X_E_NULL_PTR:
            printf("API name [%s]  Error [%d] : Null pointer\r\n", api_name, rslt);
            break;
        case BME68X_E_COM_FAIL:
            printf("API name [%s]  Error [%d] : Communication failure\r\n", api_name, rslt);
            break;
        case BME68X_E_INVALID_LENGTH:
            printf("API name [%s]  Error [%d] : Incorrect length parameter\r\n", api_name, rslt);
            break;
        case BME68X_E_DEV_NOT_FOUND:
            printf("API name [%s]  Error [%d] : Device not found\r\n", api_name, rslt);
            break;
        case BME68X_E_SELF_TEST:
            printf("API name [%s]  Error [%d] : Self test error\r\n", api_name, rslt);
            break;
        case BME68X_W_NO_NEW_DATA:
            printf("API name [%s]  Warning [%d] : No new data found\r\n", api_name, rslt);
            break;
        default:
            printf("API name [%s]  Error [%d] : Unknown error code\r\n", api_name, rslt);
            break;
    }
}

int8_t bme68x_interface_init(struct bme68x_dev *bme, uint8_t intf)
{
    int8_t rslt = BME68X_OK;
//    struct coines_board_info board_info;

    if (bme != NULL)
    {
//        int16_t result = coines_open_comm_intf(COINES_COMM_INTF_USB);
//        if (result < COINES_SUCCESS)
//        {
//            printf(
//                "\n Unable to connect with Application Board ! \n" " 1. Check if the board is connected and powered on. \n" " 2. Check if Application Board USB driver is installed. \n"
//                " 3. Check if board is in use by another application. (Insufficient permissions to access USB) \n");
//            exit(result);
//        }

//        result = coines_get_board_info(&board_info);

//#if defined(PC)
//        setbuf(stdout, NULL);
//#endif

//        if (result == COINES_SUCCESS)
//        {
//            if ((board_info.shuttle_id != BME68X_SHUTTLE_ID))
//            {
//                printf("! Warning invalid sensor shuttle \n ," "This application will not support this sensor \n");
//                exit(COINES_E_FAILURE);
//            }
//        }

        //coines_set_shuttleboard_vdd_vddio_config(0, 0);
        //nrf_delay_ms(100);

        /* Bus configuration : I2C */
        if (intf == BME68X_I2C_INTF)
        {
            //printf("I2C Interface");
            dev_addr = BME68X_I2C_ADDR_LOW;
            bme->read = bme68x_i2c_read;
            bme->write = bme68x_i2c_write;
            bme->intf = BME68X_I2C_INTF;
            //coines_config_i2c_bus(COINES_I2C_BUS_0, COINES_I2C_STANDARD_MODE);
        }
        ///* Bus configuration : SPI */
        //else if (intf == BME68X_SPI_INTF)
        //{
        //    printf("SPI Interface\n");
        //    dev_addr = COINES_SHUTTLE_PIN_7;
        //    bme->read = bme68x_spi_read;
        //    bme->write = bme68x_spi_write;
        //    bme->intf = BME68X_SPI_INTF;
        //    coines_config_spi_bus(COINES_SPI_BUS_0, COINES_SPI_SPEED_7_5_MHZ, COINES_SPI_MODE0);
        //}

        //nrf_delay_ms(100);

        //coines_set_shuttleboard_vdd_vddio_config(3300, 3300);

        nrf_delay_ms(100);

        bme->delay_us = bme68x_delay_us;
        bme->intf_ptr = &dev_addr;
        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
    }
    else
    {
        rslt = BME68X_E_NULL_PTR;
    }

    return rslt;
}

//void bme68x_coines_deinit(void)
//{
//    fflush(stdout);

//    coines_set_shuttleboard_vdd_vddio_config(0, 0);
//    coines_delay_msec(1000);

//    /* Coines interface reset */
//    coines_soft_reset();
//    coines_delay_msec(1000);
//    coines_close_comm_intf(COINES_COMM_INTF_USB);
//}



///
// force mode settings
///
/***********************************************************************/
/*                         Macros                                      */
/***********************************************************************/

/* Macro for count of samples to be displayed */
#define SAMPLE_COUNT  UINT16_C(100)

struct bme68x_conf conf;
struct bme68x_heatr_conf heatr_conf;
struct bme68x_dev bme;

void app_bme68x_init(void){
    int8_t rslt=0;

    /* Interface preference is updated as a parameter
     * For I2C : BME68X_I2C_INTF
     * For SPI : BME68X_SPI_INTF
     */
    rslt = bme68x_interface_init(&bme, BME68X_I2C_INTF);
    bme68x_check_rslt("bme68x_interface_init", rslt);

    rslt = bme68x_init(&bme);
    bme68x_check_rslt("bme68x_init", rslt);

    //rslt = bme68x_selftest_check(&bme);
    //bme68x_check_rslt("bme68x_selftest_check", rslt);

    //if (rslt == BME68X_OK)
    //{
    //    printf("Self-test passed\n");
    //}else if (rslt == BME68X_E_SELF_TEST)
    //{
    //    printf("Self-test failed\n");
    //}else {
    //        printf("Self-test failed\n");

    //}
    //while(1);
    /* Check if rslt == BME68X_OK, report or handle if otherwise */
    conf.filter = BME68X_FILTER_OFF;
    conf.odr = BME68X_ODR_NONE;
    conf.os_hum = BME68X_OS_16X;
    conf.os_pres = BME68X_OS_1X;
    conf.os_temp = BME68X_OS_4X;
    rslt = bme68x_set_conf(&conf, &bme);
    bme68x_check_rslt("bme68x_set_conf", rslt);

    /* Check if rslt == BME68X_OK, report or handle if otherwise */
    heatr_conf.enable = BME68X_DISABLE;//BME68X_ENABLE;//BME68X_DISABLE;
    heatr_conf.heatr_temp = 300;
    heatr_conf.heatr_dur = 1000;
    rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme);
    bme68x_check_rslt("bme68x_set_heatr_conf", rslt);

    printf("Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Status\n");
    
}

void* app_bme68x_meas(void){
    static uint8_t n_fields;
    static uint32_t ret_env_data[10];
    static uint8_t data_index=0;
    static uint8_t dummy_data=10;
    static uint8_t dummy_vect=0;

    struct bme68x_data data;
    int8_t rslt=0;
    uint32_t del_period;

    rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
    bme68x_check_rslt("bme68x_set_op_mode", rslt);

    /* Calculate delay period in microseconds */
    del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) + (heatr_conf.heatr_dur * 1000);
    bme.delay_us(del_period, bme.intf_ptr);

    //time_ms = coines_get_millis();

    /* Check if rslt == BME68X_OK, report or handle if otherwise */
    rslt = bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme);
    bme68x_check_rslt("bme68x_get_data", rslt);

    if(data_index<100)data_index++;
    else data_index=0;

    

    if (n_fields)
    {
#ifdef BME68X_USE_FPU
        //printf("%u, %lu, %.2f, %.2f, %.2f, %.2f, 0x%x\n",
        //       sample_count,
        //       (long unsigned int)time_ms,
        //       data.temperature,
        //       data.pressure,
        //       data.humidity,
        //       data.gas_resistance,
        //       data.status);
#else
        //printf("%d, %lu, %lu, %lu, 0x%x\n",
        //       (data.temperature / 100),
        //       (long unsigned int)data.pressure,
        //       (long unsigned int)(data.humidity / 1000),
        //       (long unsigned int)data.gas_resistance,
        //       data.status);
        //printf("%08x,%08x,%08x,%08x\n",
        //       (data.temperature / 100),
        //       (long unsigned int)data.pressure,
        //       (long unsigned int)(data.humidity / 1000),
        //       (long unsigned int)data.gas_resistance);

        ret_env_data[0]=data_index;
        ret_env_data[1]=(long unsigned int)(data.temperature / 100);
        ret_env_data[2]=(long unsigned int)data.pressure;
        ret_env_data[3]=(long unsigned int)(data.humidity / 1000);
        ret_env_data[4]=(long unsigned int)data.gas_resistance;

        ////dummy data
        //ret_env_data[1]=0x12345678;
        //ret_env_data[2]=0x9ABCDEFE;
        //ret_env_data[3]=0x12345678;
        //ret_env_data[4]=0x9ABCDEFE;

        //if(dummy_data>=20)dummy_vect=0;
        //else if(dummy_data<=10)dummy_vect=1;

        //if(dummy_vect)dummy_data++;
        //else if(!dummy_vect)dummy_data--;
        //ret_env_data[1]=dummy_data;
        //ret_env_data[2]=dummy_data+10;
        //ret_env_data[3]=dummy_data+20;
        //ret_env_data[4]=dummy_data+30;


        //printf("-%02x,%02x--------\n",data_index,dummy_data);
        //printf("%02x,%08x,%08x,%08x,%08x\n",
        //                          ret_env_data[0],
        //       (long unsigned int)ret_env_data[1],
        //       (long unsigned int)ret_env_data[2],
        //       (long unsigned int)ret_env_data[3],
        //       (long unsigned int)ret_env_data[4]);
        //printf("%02x,%08d,%08d,%08d,%08d\n",
        //                          ret_env_data[0],
        //       (long unsigned int)ret_env_data[1],
        //       (long unsigned int)ret_env_data[2],
        //       (long unsigned int)ret_env_data[3],
        //       (long unsigned int)ret_env_data[4]);


#endif
    }
    return ret_env_data;
}


///***********************************************************************/
///*                         Test code                                   */
///***********************************************************************/

//void app_bme68x_test(void)
//{
//    struct bme68x_dev bme;
//    int8_t rslt=0;
//    struct bme68x_conf conf;
//    struct bme68x_heatr_conf heatr_conf;
//    struct bme68x_data data;
//    uint32_t del_period;
//    uint32_t time_ms = 0;
//    uint8_t n_fields;
//    uint16_t sample_count = 1;

//    /* Interface preference is updated as a parameter
//     * For I2C : BME68X_I2C_INTF
//     * For SPI : BME68X_SPI_INTF
//     */
//    rslt = bme68x_interface_init(&bme, BME68X_I2C_INTF);
//    bme68x_check_rslt("bme68x_interface_init", rslt);

//    rslt = bme68x_init(&bme);
//    bme68x_check_rslt("bme68x_init", rslt);

//    //rslt = bme68x_selftest_check(&bme);
//    //bme68x_check_rslt("bme68x_selftest_check", rslt);

//    //if (rslt == BME68X_OK)
//    //{
//    //    printf("Self-test passed\n");
//    //}else if (rslt == BME68X_E_SELF_TEST)
//    //{
//    //    printf("Self-test failed\n");
//    //}else {
//    //        printf("Self-test failed\n");

//    //}
//    //while(1);
//    /* Check if rslt == BME68X_OK, report or handle if otherwise */
//    conf.filter = BME68X_FILTER_OFF;
//    conf.odr = BME68X_ODR_NONE;
//    conf.os_hum = BME68X_OS_16X;
//    conf.os_pres = BME68X_OS_1X;
//    conf.os_temp = BME68X_OS_4X;
//    rslt = bme68x_set_conf(&conf, &bme);
//    bme68x_check_rslt("bme68x_set_conf", rslt);

//    /* Check if rslt == BME68X_OK, report or handle if otherwise */
//    heatr_conf.enable = BME68X_DISABLE;//BME68X_ENABLE;
//    heatr_conf.heatr_temp = 300;
//    heatr_conf.heatr_dur = 1000;
//    rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme);
//    bme68x_check_rslt("bme68x_set_heatr_conf", rslt);

//    printf("Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Status\n");
    

    
//    while (sample_count <= SAMPLE_COUNT)
//    {
//        rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
//        bme68x_check_rslt("bme68x_set_op_mode", rslt);

//        /* Calculate delay period in microseconds */
//        del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) + (heatr_conf.heatr_dur * 1000);
//        bme.delay_us(del_period, bme.intf_ptr);

//        //time_ms = coines_get_millis();

//        /* Check if rslt == BME68X_OK, report or handle if otherwise */
//        rslt = bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme);
//        bme68x_check_rslt("bme68x_get_data", rslt);

//        if (n_fields)
//        {
//#ifdef BME68X_USE_FPU
//            //printf("%u, %lu, %.2f, %.2f, %.2f, %.2f, 0x%x\n",
//            //       sample_count,
//            //       (long unsigned int)time_ms,
//            //       data.temperature,
//            //       data.pressure,
//            //       data.humidity,
//            //       data.gas_resistance,
//            //       data.status);
//#else
//            printf("%u, %lu, %d, %lu, %lu, %lu, 0x%x\n",
//                   sample_count,
//                   (long unsigned int)time_ms,
//                   (data.temperature / 100),
//                   (long unsigned int)data.pressure,
//                   (long unsigned int)(data.humidity / 1000),
//                   (long unsigned int)data.gas_resistance,
//                   data.status);
//#endif
//            sample_count++;
//        }
//    }
//    while(1);
//    //bme68x_coines_deinit();
//    //return rslt;
//}
