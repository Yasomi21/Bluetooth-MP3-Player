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

/******************************************************************************
 * User Defines
 *****************************************************************************/
#define BAUD_RATE 9600
// 115200
/******************************************************************************
 * Main
 *****************************************************************************/
int main() {
    // Initialization //
    BOARD_Init();
    TIMER_Init();

    // UART6 SETUP //
    // Since UART1 is used by the SPI, we must use UART6
    int8_t uart6_init_status = Uart6_Init(9600);
    if (uart6_init_status == ERROR) {
        printf("Failed to initialize UART6.\n");
        while (TRUE); // Indefinite loop for good practice.
    }

    // UART RECEIVE TEST //
    // char uart_rx[50];
    // while (TRUE) {
    //     // We only want to run the function periodically at 10Hz (100ms)
    //     // if (TIMERS_GetMicroSeconds() % 100 != 0) {
    //     //     continue;
    //     // };

    //     // Receive a string from UART6 (Note: This will certainly result in data loss)
    //     // Only print if reception was successful
    //     if (Uart6_rx(uart_rx, strlen(uart_rx)) == SUCCESS) {
    //         // Apply termination character to reduce overflow
    //         if (strlen(uart_rx) <= 1) {
    //             continue;
    //         }
    //         uart_rx[strlen(uart_rx)] = '\0';

    //         // Display string
    //         printf("%s", uart_rx);
    //     }
    // }

    // LOOPBACK UART TEST (Connect TX of UART6 to RX of UART6) //
    // int counter = 0;
    // while (TRUE) {
    //     // We only want to run the function periodically at 10Hz (100ms)
    //     if (TIMERS_GetMilliSeconds() % 1000 != 0) {
    //         continue;
    //     };

    //     // Transmit a string from UART6
    //     char uart_tx[50];
    //     sprintf(uart_tx, "A: %d\r\n", counter);
    //     uart_tx[strlen(uart_tx)] = '\0';
    //     // If transmission fails, try again in the next iteration
    //     if (Uart6_tx(uart_tx, strlen(uart_tx)) == ERROR) {
    //         continue;
    //     } else {
    //         counter++;
    //     }

    //     // Receive a string from UART6
    //     char uart_rx[50];
    //     // Only print if reception was successful
    //     if (Uart6_rx(uart_rx, strlen(uart_tx)) == SUCCESS) {
    //         // Apply termination character to reduce overflow
    //         uart_rx[strlen(uart_tx)] = '\0';

    //         // Display string
    //         printf("%s", uart_rx);

    //         // Increment counter to check for UART updates.
    //         counter++;
    //     }
    // }

    // UART TRANSMISSION TEST (was able to be detected by the App's UART) // 
    int counter = 0;
    while (TRUE) {
        // We only want to run the function periodically at 10Hz (100ms)
        if (TIMERS_GetMilliSeconds() % 1000 == 0) {
            // Transmit a string from UART6
            char uart_tx[50];
            sprintf(uart_tx, "A: %d\r\n", counter);
            uart_tx[strlen(uart_tx)] = '\0';
            // If transmission fails, try again in the next iteration
            if (Uart6_tx(uart_tx, strlen(uart_tx)) == ERROR) {
                continue;
            } else {
                counter++;
            }
        };
    }



    return SUCCESS;
}