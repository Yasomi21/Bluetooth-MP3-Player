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

// Forward declaration
void send_packet(char *data, int len);
unsigned char calc_checksum(unsigned char charIn, unsigned char curChecksum);

unsigned char calc_checksum(unsigned char charIn, unsigned char curChecksum){
    // Right shift the current checksum by 1 and add the result to the left-shifted current checksum
    curChecksum = (curChecksum >> 1) + (curChecksum << 7);
    // Add the new character to the checksum
    curChecksum += charIn;
    return curChecksum;
}


void send_packet(char *data, int len) {

    // Head
    uint8_t head = 0xCC;
    BLE_PutChar(head);

    // Length
    uint8_t length = len;
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

int cap1_count = 0;
int cap1_trigs = 0;

int cap2_count = 0;
int cap2_trigs = 0;

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

    PWM_AddPin(PWM_1); // Red
    PWM_AddPin(PWM_3); // Green
    PWM_AddPin(PWM_5); // Blue


    while (TRUE) {
        
        PWM_SetDutyCycle(PWM_1, 100);
        PWM_SetDutyCycle(PWM_3, 0);
        PWM_SetDutyCycle(PWM_5, 0);

        // Run Loop for UART
        BLE_RunLoop();

        unsigned int piezo = ADC_Read(ADC_2) * 2; // This is reading the values from the piezo sensor
        bool rotButton = QEI_ButtonStatus(); // This is reading the values from the rotary encoder button
        unsigned int rot = QEI_GetPosition(); // This is reading the values from the rotary encoder

        unsigned int ct1_state = CAPTOUCH1_IsTouched();
        if (ct1_state){
            // Count the triggers
            cap1_trigs++;
        }

        cap1_count++;

        if (cap1_count > 500) { // Next Song
            if (cap1_trigs > 450) {
                if (cap1_state == FALSE) {
                    cap1_state = TRUE;
                    char ch[2];
                    ch[0] = 1;
                    ch[1] = 65;
                    send_packet(ch, 2);
                    printf("captouch1\n");
                }
            } else if (cap1_state == TRUE) {
                cap1_state = FALSE;
            }

            cap1_count = 0;
            cap1_trigs = 0;
        }
        // if (ct1_state && (cap1_state == FALSE)) { // Next Song
        //     cap1_state = TRUE;
        //     char ch[1];
        //     ch[0] = 5;
        //     //send_packet(ch);
        //     printf("cap1 touched.\n");
        //     nextSong = 1;
        //     //return;
        // } else if (!ct1_state && (cap1_state == TRUE)) {
        //     cap1_state = FALSE;
        // }
        
        unsigned int ct2_state = CAPTOUCH2_IsTouched();
        if (ct2_state){
            // Count the triggers
            cap2_trigs++;
        }

        cap2_count++;

        if (cap2_count > 100) { // Prev Song
            if (cap2_trigs > 95) {
                if (cap2_state == FALSE) {
                    cap2_state = TRUE;
                    char ch[2];
                    ch[0] = 2;
                    ch[1] = 66;
                    send_packet(ch, 2);
                    printf("captouch2\n");
                }
            } else if (cap2_state == TRUE) {
                cap2_state = FALSE;
            }

            cap2_count = 0;
            cap2_trigs = 0;
        }
        // if (ct2_state && (cap2_state == FALSE)){ // Previous Song
        //     cap2_state = TRUE;
        //     char ch[1];
        //     ch[0] = 8;
        //     printf("cap2 touched.\n");
        //     //send_packet(ch);
        //     prevSong = 1;
        //     //return;
        // } else if (!ct2_state && (cap2_state == TRUE)) {
        //     cap2_state = FALSE;
        // }
        if (rot > 0){ // Scrolling up
            Scroll_Up = 1;
        } else if (rot < 0){ // Scrolling Down
            Scroll_Down = 1; 
        } else if (rot == 20) { // Once reach the end of the music library
            QEI_ResetPosition();
        }
        
        if (rotButton == 1) { // Rotary Encoder is pressed
            selectMusic = 1;
            PWM_SetDutyCycle(PWM_1, 0);
            PWM_SetDutyCycle(PWM_3, 100);
            PWM_SetDutyCycle(PWM_5, 0);
        }    

        if (piezo > 70) { // PIEZO is Touched
            //HAL_Delay(350);
            if (Music_status == 1 && selectMusic == 1){
                Music_status = 0;
                PWM_SetDutyCycle(PWM_1, 100);
                PWM_SetDutyCycle(PWM_3, 0);
                PWM_SetDutyCycle(PWM_5, 0);
            } else {
                Music_status = 1;
                PWM_SetDutyCycle(PWM_1, 0);
                PWM_SetDutyCycle(PWM_3, 100);
                PWM_SetDutyCycle(PWM_5, 0);
            }
        }
    }

    BOARD_End();
    return 0;
}
