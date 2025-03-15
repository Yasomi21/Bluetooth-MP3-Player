/******************************************************************************
 * Libraries
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * User Libraries
 *****************************************************************************/
#include "Board.h"
#include "uart.h"
#include "timers.h"
#include "leds.h"
#include "bluefruit_ble_uart.h"

/******************************************************************************
 * User Defines
 *****************************************************************************/

/******************************************************************************
 * Main
 *****************************************************************************/
#define MAIN
#ifdef MAIN
int main() {

    // Initialization
    BOARD_Init();
    TIMER_Init();
    LEDS_Init();

    uint8_t ble_status = BLE_UART_Init();
    if (ble_status == ERROR) {
        set_leds(0xFF);
    }

    
    int previous = 0;
    while (TRUE) {
        BLE_RunLoop();
        //set_leds(tx_buffer.tail);

        // If a character is present, print it out...?
        uint8_t x;
        uint8_t status = BLE_GetChar(&x);
        if (status == SUCCESS) {
            // BLE_PutChar(x); // Loopback test, put the character and send it.
            printf("Msg: %c\n", x); // UART is set to have just a newline ending, so don't need to put it there, '\n'
        }

        int delta = TIMERS_GetMilliSeconds() - previous;
        if (delta > 1000) {
            uint8_t ch = 'Q';
            BLE_PutChar(ch);
            previous = TIMERS_GetMilliSeconds();
        }
    }

    return 1;
}
#endif