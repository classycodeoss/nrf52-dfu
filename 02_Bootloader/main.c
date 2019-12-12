//
//  main.c
//  nrf52-dfu
//
//  Sample bootloader implementation.
//
//  Created by Andreas Schweizer on 26.04.2019.
//  Copyright © 2019 Classy Code GmbH
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be included in all copies
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <stdint.h>
#include "boards.h"
#include "custom_board.h"
#include "nrf_mbr.h"
#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_dfu_timers.h"
#include "nrf_dfu.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"
#include "nrf_power.h"
#include "nrf_dfu_utils.h"
#include "nrf_drv_uart.h"

static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();

#if NRF_MODULE_ENABLED(NRF_LOG_BACKEND_RTT)
    // To allow the buffer to be flushed by the host.
    nrf_delay_ms(100);
#endif
#ifdef NRF_DFU_DEBUG_VERSION
    NRF_BREAKPOINT_COND;
#endif
    NVIC_SystemReset();
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    NRF_LOG_ERROR("%s:%d", p_file_name, line_num);
    on_error();
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}

void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
    on_error();
}

static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
    switch (evt_type)
    {
        case NRF_DFU_EVT_DFU_FAILED:
            break;
        case NRF_DFU_EVT_DFU_ABORTED:
            break;
        case NRF_DFU_EVT_DFU_INITIALIZED:
            break;
        case NRF_DFU_EVT_TRANSPORT_ACTIVATED:
            break;
        case NRF_DFU_EVT_DFU_STARTED:
            break;
        default:
            break;
    }
}

char gMsgHelloFromBootloader[] = "@@BOOTLOADER\r\n";
char gMsgDfuRequested[] = "@@DFUR >\r\n";

void printWelcomeMessage()
{
    nrf_drv_uart_t uart_inst = NRF_DRV_UART_INSTANCE(0);
    nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;
    uart_config.pselrxd = RX_PIN_NUMBER;
    uart_config.pseltxd = TX_PIN_NUMBER;
    uart_config.pselcts = CTS_PIN_NUMBER;
    uart_config.pselrts = RTS_PIN_NUMBER;
    uart_config.p_context = NULL;
    uart_config.hwfc = NRF_UART_HWFC_DISABLED;
    uart_config.parity = NRF_UART_PARITY_EXCLUDED;
    uart_config.baudrate = NRF_UART_BAUDRATE_57600;
    uart_config.interrupt_priority = 3;

    uint32_t ret_val = nrf_drv_uart_init(&uart_inst, &uart_config, NULL);
    if (ret_val == NRF_SUCCESS) {
        nrf_drv_uart_tx(&uart_inst, (const uint8_t *)gMsgHelloFromBootloader, strlen(gMsgHelloFromBootloader));
        // DFU requested via GPREGRET.
        if (nrf_power_gpregret_get() & 0xB1) {
            nrf_drv_uart_tx(&uart_inst, (const uint8_t *)gMsgDfuRequested, strlen(gMsgDfuRequested));
        }
        nrf_delay_ms(10);
        nrf_drv_uart_uninit(&uart_inst);
    }
}

int main(void)
{
    uint32_t ret_val;

    // Protect MBR and bootloader code from being overwritten.
    ret_val = nrf_bootloader_flash_protect(0, MBR_SIZE, false);
    APP_ERROR_CHECK(ret_val);
    ret_val = nrf_bootloader_flash_protect(BOOTLOADER_START_ADDR, BOOTLOADER_SIZE, false);
    APP_ERROR_CHECK(ret_val);

    printWelcomeMessage();
    
    (void) NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("Inside main");

    ret_val = nrf_bootloader_init(dfu_observer);
    APP_ERROR_CHECK(ret_val);

    // Either there was no DFU functionality enabled in this project or the DFU module detected
    // no ongoing DFU operation and found a valid main application.
    // Boot the main application.
    nrf_bootloader_app_start();

    // Should never be reached.
    NRF_LOG_INFO("After main");
}
