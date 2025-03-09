#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Board.h>
#include "uart.h"
#include "timers.h"

//#define MAIN
#ifdef MAIN
int main(void) {

    int baud_rate = 115200;
    // Initialization
    BOARD_Init();
    TIMER_Init();
    char u1_init_success = Uart1_Init(baud_rate);
    char u6_init_success = Uart6_Init(baud_rate);

    if ((u1_init_success == ERROR) || (u6_init_success == ERROR)) {
        //printf("Uart Failed to Initialize.\r\n");
        return;
    }
    
    //char tx[50];
    int counter = 0;
    while (TRUE) {

        if (TIMERS_GetMilliSeconds() % 100 != 0) {
            continue;
        } 
        
        // char tx[50] = "Hello\r\n";
        // if (Uart1_tx(tx, strlen(tx) - 1) == SUCCESS) {
        //     char rx[50];
        //     Uart6_rx(rx, strlen(tx) - 1);
        //     printf("%s", rx);
        //     //printf("%d\n", strlen(x));
        // }

        // So far, it seems like the counter is working? There's a weird space though
        // char tx[50];
        // sprintf(tx, "Counter: %d\r\n", counter);
        // if (Uart1_tx(tx, strlen(tx)) == SUCCESS) {
        //     char rx[50];
        //     Uart6_rx(rx, strlen(tx));
        //     printf("%s", rx);
        //     //printf("%d\n", strlen(x));
        // }
        // counter++;


        // Seperate transmit and receive.
        // One part will handle transmission and error handling.
        // The other receive part will check if reception was correct.
        char tx[50];
        sprintf(tx, "Counter: %d\r\n", counter);
        if (Uart1_tx(tx, strlen(tx)) == ERROR) {
            continue;
        }

        //DelayMicros(100000);
        
        char rx[50];
        if (Uart6_rx(rx, strlen(tx)) == SUCCESS) {
            rx[strlen(tx)] = '\0'; // Add termination bit at the end to prevent overflow
            printf("%s", rx);
            //printf("%d\n", counter);
            counter++;
        }
        //printf("%d\n", strlen(x));
        
    }
}
#endif