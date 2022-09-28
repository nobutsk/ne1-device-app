#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf.h"
#include "nrf_soc.h"
#include "nordic_common.h"
#include "boards.h"
#include "app_timer.h"
#include "app_util.h"
#include "nrf_fstorage.h"

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "ecg_define.h"
#include "flash_ctrl.h"

///* Dummy data to write to flash. */
//static uint32_t m_data          = 0xBADC0FFE;
//static char     m_hello_world[] = "hello world";

nvm_data_block_t nvm_data;

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    //.start_addr = 0x3e000,
    //.end_addr   = 0x3ffff,
    .start_addr = 0x6e000,
    .end_addr   = 0x6ffff,

};



/**@brief   Helper function to obtain the last address on the last page of the on-chip flash that
 *          can be used to write user data.
 */
static uint32_t nrf5_flash_end_addr_get()
{
    uint32_t const bootloader_addr = BOOTLOADER_ADDRESS;
    uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
    uint32_t const code_sz         = NRF_FICR->CODESIZE;
    NRF_LOG_INFO("bootloader_address=%x,codepageSize=%x,Codesize=%x",bootloader_addr,page_sz,code_sz);
    NRF_LOG_INFO("==============================");
    return (bootloader_addr != 0xFFFFFFFF ?
            bootloader_addr : (code_sz * page_sz));
}

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}


static void print_flash_info(nrf_fstorage_t * p_fstorage)
{
    NRF_LOG_INFO("========| flash info |========");
    NRF_LOG_INFO("erase unit: \t%d bytes",      p_fstorage->p_flash_info->erase_unit);
    NRF_LOG_INFO("program unit: \t%d bytes",    p_fstorage->p_flash_info->program_unit);
    NRF_LOG_INFO("==============================");
}


void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        (void) sd_app_evt_wait();
    }
}

void flash_init(void){
    ret_code_t rc;

    nrf_fstorage_api_t * p_fs_api;
    NRF_LOG_INFO("Initializing nrf_fstorage_sd");
    NRF_LOG_INFO("erase flash threshold=%xto%x",FIRST_ADDR,(FIRST_ADDR+NVM_MAX_INDEX*NVM_DATA_SIZE));
    /* Initialize an fstorage instance using the nrf_fstorage_sd backend.
     * nrf_fstorage_sd uses the SoftDevice to write to flash. This implementation can safely be
     * used whenever there is a SoftDevice, regardless of its status (enabled/disabled). */
    p_fs_api = &nrf_fstorage_sd;

    rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);

    print_flash_info(&fstorage);

    /* It is possible to set the start and end addresses of an fstorage instance at runtime.
     * They can be set multiple times, should it be needed. The helper function below can
     * be used to determine the last address on the last page of flash memory available to
     * store data. */
    (void) nrf5_flash_end_addr_get();
}



nvm_data_block_t flash_read_pw_on(void){
    nvm_data.index_addr=FIRST_ADDR;
    nrf_fstorage_read(&fstorage, nvm_data.index_addr, nvm_data.curr, sizeof(nvm_data.curr));
    wait_for_flash_ready(&fstorage);
    //NRF_LOG_INFO("val=%x,%x,%x",next_data[0],next_data[1],next_data[2]);
    //memcpy(curr_data,next_data,4);
    //NRF_LOG_INFO("val=%x,%x,%x",curr_data[0],curr_data[1],curr_data[2]);

    switch (nvm_data.curr[NVM_DATA_SIZE-1]){
      case NVM_FLAG:
        NRF_LOG_INFO("data exist");
        NRF_LOG_INFO("index_addr=%x,data=%x",nvm_data.index_addr,nvm_data.curr[0]);

        flash_search_curr_addr();
      break;
      case 0xFF:
        NRF_LOG_INFO("no data");
        //use init value 
      break;
      default:
      break;
    }

    NRF_LOG_INFO("val=%x,%x,%x",nvm_data.curr[0],nvm_data.curr[1],nvm_data.curr[2],nvm_data.curr[3]);
    return nvm_data;

}
void flash_search_curr_addr(void){
    
    for(uint16_t i=0;i<=NVM_MAX_INDEX;i++){
        nvm_data.index_addr=nvm_data.index_addr+NVM_DATA_SIZE;    //next data
        nrf_fstorage_read(&fstorage, nvm_data.index_addr, nvm_data.next, sizeof(nvm_data.next));
        wait_for_flash_ready(&fstorage);
        if(nvm_data.next[NVM_DATA_SIZE-1]==NVM_FLAG){
            NRF_LOG_INFO("data exist");
            NRF_LOG_INFO("index_addr=%x,data=%x",nvm_data.index_addr,nvm_data.curr[0]);
            memcpy(nvm_data.curr,nvm_data.next,NVM_DATA_SIZE);
        }else if(nvm_data.next[NVM_DATA_SIZE-1]==0xFF){
            NRF_LOG_INFO("no data");
            return;
        }
    }
}

void flash_write_before_pw_off(void){
    ret_code_t rc;

    if((FIRST_ADDR+NVM_MAX_INDEX*NVM_DATA_SIZE)<nvm_data.index_addr){

        NRF_LOG_INFO("erase flash.(First Addr + 0x1000)");
        rc = nrf_fstorage_erase(&fstorage, FIRST_ADDR, ERASE_PAGE_SIZE, NULL);
        APP_ERROR_CHECK(rc);
        wait_for_flash_ready(&fstorage);

        NRF_LOG_INFO("Writing \"%x\" to flash.", nvm_data.curr);
        NRF_LOG_HEXDUMP_INFO(nvm_data.curr,NVM_DATA_SIZE);
        nvm_data.curr[NVM_INDEX]=0;
        rc = nrf_fstorage_write(&fstorage, FIRST_ADDR, nvm_data.curr, sizeof(nvm_data.curr), NULL);
        APP_ERROR_CHECK(rc);
    }else{
        NRF_LOG_INFO("Writing \"%x\" to flash.", nvm_data.curr);
        nvm_data.curr[NVM_INDEX]++;
        nvm_data.curr[NVM_DATA_SIZE-1]=NVM_FLAG;
        rc = nrf_fstorage_write(&fstorage, nvm_data.index_addr, &nvm_data.curr, sizeof(nvm_data.curr), NULL);
        APP_ERROR_CHECK(rc);

        wait_for_flash_ready(&fstorage);

        //NRF_LOG_INFO("flash_write_before_pw_off Done.");    
    }
}

void flash_erase(void){
    ret_code_t rc;
    NRF_LOG_INFO("erase flash.(First Addr + 0x1000)");
    rc = nrf_fstorage_erase(&fstorage, FIRST_ADDR, ERASE_PAGE_SIZE, NULL);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);
}