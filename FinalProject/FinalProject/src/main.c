#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <pwm.h>
#include <timers.h>
#include <QEI.h>
#include <ADC.h>
#include <stdbool.h>

#include <CAPTOUCH1.h>
#include <CAPTOUCH2.h>

#include "bluefruit_ble_uart.h"

#define DELAY(x)    {int wait; for (wait = 0; wait <= x; wait++) {asm("nop");}}
#define A_BIT       18300
#define A_LOT       183000

int glob_time_micro = 0;

// typedef enum {
//     NOSENSOR,
//     CAPRIGHTTOUCH, // Captouch 1
//     CAPLEFTTOUCH,  // Captouch 2
//     ROTPRESS,
//     ROTINCREASE,
//     ROTDECREASE,
//     PIEZOTOUCH,
// }sensorStatus;

// Forward declaration
void send_packet(char *data);
unsigned char calc_checksum(unsigned char charIn, unsigned char curChecksum);

unsigned char calc_checksum(unsigned char charIn, unsigned char curChecksum){
    // Right shift the current checksum by 1 and add the result to the left-shifted current checksum
    curChecksum = (curChecksum >> 1) + (curChecksum << 7);
    // Add the new character to the checksum
    curChecksum += charIn;
    return curChecksum;
}


void send_packet(char *data) {

    // Head
    uint8_t head = 0xCC;
    BLE_PutChar(head);

    // Length
    uint8_t length = 5;
    BLE_PutChar(length);

    // Payload
    char checksum = 0;
    for (int i = 0; i < length; i++) {
        // Insert the character
        char ch = data[i];
        BLE_PutChar(ch);

        printf("Character payload is %c. \n", ch);
        // Calculate the checksum
        checksum = calc_checksum(ch, checksum);
    }
    // uint8_t id = 0x00;
    // //uint8_t d1 = 0x84; //0x84
    // BLE_PutChar(id);

    // uint8_t d2 = 0x00; //0x00
    // BLE_PutChar(d2);

    // uint8_t d3 = 0x25; //0x25
    // BLE_PutChar(d3);

    // uint8_t d4 = 0x7D; //0x7D
    // BLE_PutChar(d4);

    // uint8_t d5 = 0x96; //0x96
    // BLE_PutChar(d5);

    uint8_t tail = 0xB9;
    BLE_PutChar(tail);

    // uint8_t checksum = 0x1D; //0xE6; // Old known checksum when ID was 0x84
    BLE_PutChar(checksum);

    uint8_t RC = '\r';
    uint8_t NL = '\n';
    BLE_PutChar(RC);
    BLE_PutChar(NL);

}

int nextSong = 0;
int prevSong = 0;
int Music_status = 0; // 0 is pause, 1 is play
int selectMusic = 0;
int Scroll_Up = 0;
int Scroll_Down = 0;

bool cap1_state = FALSE;
bool cap2_state = FALSE;

int main(void) {
    // Initialize all modules
    PWM_Init();
    BOARD_Init();
    CAPTOUCH1_Init();
    CAPTOUCH2_Init();
    QEI_Init();
    TIMER_Init();
    ADC_Init();
    BLE_UART_Init();

    while (TRUE) {
        // Run Loop for UART
        BLE_RunLoop();

        unsigned int piezo = ADC_Read(ADC_2) * 2; // This is reading the values from the piezo sensor
        // bool rotButton = QEI_ButtonStatus(); // This is reading the values from the rotary encoder button
        // rotButton = rotButton << 1;
        unsigned int rot = QEI_GetPosition(); // This is reading the values from the rotary encoder
        //char first_touch = CAPTOUCH1_IsTouched();
        // char second_touch = CAPTOUCH2_IsTouched();
        //  if (first_touch){
        //     printf("touch!\n");
        //  }else{
        //     printf("nottouch!\n");
        //  }
        // prevState = currentState;

        // switch(currentState){
        //     case NOSENSOR:
        //         if (CAPTOUCH1_IsTouched()){
        //             currentState = CAPRIGHTTOUCH;
        //         } else if (CAPTOUCH2_IsTouched()){
        //             currentState = CAPLEFTTOUCH;
        //         } else if (QEI_GetPosition() > 0){
        //             currentState = ROTINCREASE;
        //         } else if (QEI_GetPosition() < 0){
        //             currentState = ROTDECREASE;
        //         }
        //         break;
        //     case CAPRIGHTTOUCH:
        //         break;
        //     case CAPLEFTTOUCH:
        //         break;
        //     case ROTPRESS:
        //         break;
        //     case ROTINCREASE:
        //         break;
        //     case ROTDECREASE:
        //         break;
        //     case PIEZOTOUCH:
        //         break;
        //     default:
        //         break;

        unsigned int ct1_state = CAPTOUCH1_IsTouched();
        if (ct1_state && (cap1_state == FALSE)) { // Next Song
            cap1_state = TRUE;
            char ch[1];
            ch[0] = 5;
            //send_packet(ch);
            printf("cap1 touched.\n");
            nextSong = 1;
            //return;
        } else if (!ct1_state && (cap1_state == TRUE)) {
            cap1_state = FALSE;
        }
        
        unsigned int ct2_state = CAPTOUCH2_IsTouched();
        if (ct2_state && (cap2_state == FALSE)){ // Previous Song
            cap2_state = TRUE;
            char ch[1];
            ch[0] = 8;
            printf("cap2 touched.\n");
            //send_packet(ch);
            prevSong = 1;
            //return;
        } else if (!ct2_state && (cap2_state == TRUE)) {
            cap2_state = FALSE;
        }
        if (rot > 0){ // Scrolling up
            Scroll_Up = 1;
            //return;
        } else if (rot < 0){ // Scrolling Down
            Scroll_Down = 1; 
            //return;
        } 
        
        // if () { // Add a case for pressed
        // }    

        if (piezo > 70) { // PIEZO is Touched
            HAL_Delay(350);
            if (Music_status == 0){
                Music_status = 1;
            } else {
                Music_status = 0;
            }
            //return;
        }
        // printf("Button Status: %d\n\r", rotButton);
    }

    BOARD_End();
    return 0;
}
