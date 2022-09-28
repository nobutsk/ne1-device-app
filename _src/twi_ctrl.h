#ifndef TWI_CTRL_H__
#define TWI_CTRL_H__


void twi_init(void);
void twi_test(void);
void twi_tx(uint8_t addr,uint8_t tx_len);
void twi_rx(uint8_t addr,uint8_t rx_len);
uint8_t twi_battery_State_Of_Charge_get(void);

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);


#endif