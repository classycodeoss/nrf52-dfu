//
//  main.c
//  nrf52-dfu
//
//  Sample application to demonstrate the firmware update process.
//
//  If APP_VERSION is set to 1, the LED will blink 3 times, then pause and blink again.
//  If APP_VERSION is set to 2, the LED will blink 4 times, then pause and blink again.
//  If the user presses a button, the app will trigger a firmware update via bootloader.
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
#include "nrf_gpio.h"
#include "nrf_sdh.h"
#include "nrf_power.h"
#include "nrf_pwr_mgmt.h"
#include "app_timer.h"


// ANNA-B112 Dev Kit: IO_29, P0.27 -- Modify for your own target hardware!
#define LED 27

// Modify for your board!
// IO_38 = P0.24 = SW2
// IO_30 = P0.25 = SW1
#define BTN 24


// An '1' in the pattern corresponds to 'led on', a '0' to 'led off'.
#if APP_VERSION == 2
#  define LED_PATTERN 0xaa00
#else
#  define LED_PATTERN 0xa800
#endif
#define LED_PATTERN_LEN 10
static uint16_t sLedPattern;
static uint8_t sLedPatternIx;


APP_TIMER_DEF(m_timer_tick_src_id);


static void setLed(uint8_t on);
static void timers_init(void);
static void timer_tick_callback(void *p_context);


int main(void)
{
    nrf_gpio_cfg_output(LED);

    // ANNA-B112 Dev Kit: pull ups, switches pull to GND
    nrf_gpio_cfg_input(BTN, NRF_GPIO_PIN_PULLUP);

    timers_init();
    
    ret_code_t err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    for (;;)
    {
        // Sleeps until next the next event occurs.
        nrf_pwr_mgmt_run();
    }
}

static void setLed(uint8_t on)
{
    if (on) {
        nrf_gpio_pin_clear(LED);
        return;
    }
    nrf_gpio_pin_set(LED);
}

static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_create(&m_timer_tick_src_id, APP_TIMER_MODE_REPEATED, timer_tick_callback);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_timer_tick_src_id, APP_TIMER_TICKS(150), NULL);
    APP_ERROR_CHECK(err_code);
}

static void timer_tick_callback(void *p_context)
{
    setLed((sLedPattern & 0x8000) ? 1 : 0);
    
    // Next time, we'll use the next bit in the pattern.
    sLedPattern = sLedPattern << 1;
    
    // Restart the pattern if it's used up.
    if (++sLedPatternIx >= LED_PATTERN_LEN) {
        sLedPattern = LED_PATTERN;
        sLedPatternIx = 0;
    }
    
    // Poll the button to check if we should trigger a firmware update.
    int btnPressed = !nrf_gpio_pin_read(BTN);
    if (btnPressed) {
        setLed(1);
        // 0xB1 tells the bootloader to enter DFU mode.
        nrf_power_gpregret_set(nrf_power_gpregret_get() | 0xB1);
        sd_nvic_SystemReset(); // should not return
    }
}
