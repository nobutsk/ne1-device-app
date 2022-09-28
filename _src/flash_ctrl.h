#ifndef FLASH_CTRL_H__
#define FLASH_CTRL_H__


#define FIRST_ADDR 0x6e000
#define NVM_MAX_INDEX 0x100
#define NVM_FLAG      0xF0
#define NVM_DATA_SIZE 8
#define ERASE_PAGE_SIZE 0x01  //0x1000

#define NVM_INDEX       0
#define NVM_BATT_SAVE   1

typedef struct{
    uint32_t index_addr;
    uint8_t curr[NVM_DATA_SIZE];
    uint8_t next[NVM_DATA_SIZE];
} nvm_data_block_t;


void flash_init(void);
nvm_data_block_t flash_read_pw_on(void);
void flash_write_before_pw_off(void);
void flash_search_curr_addr(void);
void flash_erase(void);

extern nvm_data_block_t nvm_data;
//void flash_nvm_write(uint32_t addr,uint32_t data,uint8_t len);
//void flash_nvm_read(uint32_t addr,uint32_t data,uint8_t len);
#endif