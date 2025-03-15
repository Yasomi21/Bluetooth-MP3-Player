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
// #define MAIN
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

    printf("Reception Test:\n");
    int previous = 0;
    while (TRUE) {
        BLE_RunLoop();
        //set_leds(tx_buffer.tail);

        // If a character is present, print it out...?
        // uint8_t x;
        // uint8_t status = BLE_GetChar(&x);
        // if (status == SUCCESS) {
        //     //printf("h\n");
        //     printf("Msg: %c\n", x); // UART is set to have just a newline ending, so don't need to put it there, '\n'
        // }

        // Transmission Test (Packets)
        int delta = TIMERS_GetMilliSeconds() - previous;
        if (delta > 1000) {

            printf("Sending packet from STM32.\n");

            // Every second, construct a packet with a known checksum and send it.
            // Let's do: 0x8400257D96 → 0xE6 (Known Checksum) This is from ECE121's Lab 1 Checksum Example
            // 0x84 0x00 0x25 0x7D 0x96
            /*
            AWAIT_HEAD - Checks for the head byte to begin packet building.
            AWAIT_LENGTH - Wait for the next byte, assuming its the length.
            AWAIT_PAYLOAD - Will keep taking data bytes in this state until it receives a tail.
            AWAIT_CHKSUM - Waits for the checksum to compare with the computed checksum.
            AWAIT_END_RC - Waits for the return carriage '\r' in the first part to signify the end of transmission.
            AWAIT_END_NL - Waits for the newline '\n' as the final part to represent end of transmission and to form the full packet.
            */

            // Head
            uint8_t head = 0xCC;
            BLE_PutChar(head);

            // Length
            uint8_t length = 5;
            BLE_PutChar(length);

            // Payload
            uint8_t id = 0x00;
            //uint8_t d1 = 0x84; //0x84
            BLE_PutChar(id);

            uint8_t d2 = 0x00; //0x00
            BLE_PutChar(d2);

            uint8_t d3 = 0x25; //0x25
            BLE_PutChar(d3);

            uint8_t d4 = 0x7D; //0x7D
            BLE_PutChar(d4);

            uint8_t d5 = 0x96; //0x96
            BLE_PutChar(d5);

            uint8_t tail = 0xB9;
            BLE_PutChar(tail);

            uint8_t checksum = 0x1D; //0xE6; // Old known checksum when ID was 0x84
            BLE_PutChar(checksum);

            uint8_t RC = '\r';
            uint8_t NL = '\n';
            BLE_PutChar(RC);
            BLE_PutChar(NL);
            
            previous = TIMERS_GetMilliSeconds();
        }
    }

    return 1;
}
#endif