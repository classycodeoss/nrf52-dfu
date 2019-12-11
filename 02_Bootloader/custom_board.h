#ifndef BOARD_CUSTOM_H
#define BOARD_CUSTOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

#define LEDS_NUMBER    0

#define LED_START      0
// #define LED_1          27 // red
#define LED_STOP       0

#define LEDS_ACTIVE_STATE 0

#define LEDS_INV_MASK  LEDS_MASK

#define LEDS_LIST { }

// #define BSP_LED_0      LED_1

#define BUTTONS_NUMBER 0

#define BUTTON_START   0
// #define BUTTON_1       24
#define BUTTON_STOP    0
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { }

// #define BSP_BUTTON_0   BUTTON_1

#define RX_PIN_NUMBER  2
#define TX_PIN_NUMBER  3
#define RTS_PIN_NUMBER 0xFFFFFFFF
#define CTS_PIN_NUMBER 0xFFFFFFFF
#define HWFC           false

#ifdef __cplusplus
}
#endif

#endif // BOARD_CUSTOM_H
