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
unsigned char packet_count = 0;

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
int Scroll_Up = 0;
int Scroll_Down = 0;

bool cap1_state = FALSE;
bool cap2_state = FALSE;

int cap1_count = 0;
int cap1_trigs = 0;

int cap2_count = 0;
int cap2_trigs = 0;

bool music_state = false; // Paused state

unsigned int last_piezo = 0;
unsigned int last_Cap1 = 0;
unsigned int last_Cap2 = 0;
unsigned int last_rot = 0;
unsigned int prevRot = 0;

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
        
    PWM_SetDutyCycle(PWM_1, 65);
    PWM_SetDutyCycle(PWM_3, 100);
    PWM_SetDutyCycle(PWM_5, 0);

    while (TRUE) {
        // Run Loop for UART
        BLE_RunLoop();

        bool rotButton = QEI_ButtonStatus(); // This is reading the values from the rotary encoder button
        int rot = QEI_GetPosition(); // This is reading the values from the rotary encoder

        unsigned int ct1_state = CAPTOUCH1_IsTouched();
        if (ct1_state){
            // Count the triggers
            cap1_trigs++;
        }

        cap1_count++;

        if (cap1_count > 500) { // Next Song
            if (cap1_trigs > 450) {
                // Trigger after debounce
                int delta = TIMERS_GetMilliSeconds() - last_Cap1;
                if (delta > 250) {
                    if (cap1_state == FALSE) {
                        cap1_state = TRUE;
                        char ch[2];
                        ch[0] = 4; // Message ID
                        ch[1] = 65;
                        send_packet(ch, 2);
                        printf("captouch1\n");
                    }
                }
                last_Cap1 = TIMERS_GetMilliSeconds();
            } else if (cap1_state == TRUE) {
                cap1_state = FALSE;
            }

            cap1_count = 0;
            cap1_trigs = 0;
        }
        
        unsigned int ct2_state = CAPTOUCH2_IsTouched();
        if (ct2_state){
            // Count the triggers
            cap2_trigs++;
        }

        cap2_count++;

        if (cap2_count > 100) { // Prev Song
            if (cap2_trigs > 95) {
                // Trigger after debounce
                int delta = TIMERS_GetMilliSeconds() - last_Cap2;
                if (delta > 250) {
                    if (cap2_state == FALSE) {
                        cap2_state = TRUE;
                        char ch[2];
                        ch[0] = 5; // Message ID
                        ch[1] = 66;
                        send_packet(ch, 2);
                        printf("captouch2\n");
                    }
                }
                // Update the time
                last_Cap2 = TIMERS_GetMilliSeconds();
            } else if (cap2_state == TRUE) {
                cap2_state = FALSE;
            }

            cap2_count = 0;
            cap2_trigs = 0;
        }

        if (rot > prevRot){ // Scrolling up
            prevRot = rot;
            char t = TIMERS_GetMilliSeconds();
            char ch[4];
            ch[0] = 1; // Message ID
            ch[1] = packet_count;
            ch[2] = t >> 8; // Uper half
            ch[3] = t; // Lower half

            send_packet(ch, 4);
            printf("scrolling up\n");
            packet_count++;
        } else if (rot < prevRot){ // Scrolling Down
            prevRot = rot;
            char ch[2];
            ch[0] = 2; // Message ID
            ch[1] = 65;
            send_packet(ch, 2);
            printf("scrolling down\n");
        }
        
        if (rotButton == 1) { // Rotary Encoder is pressed
            music_state = true;
            // Trigger after debounce
            int delta = TIMERS_GetMilliSeconds() - last_rot;
            if (delta > 500) {
                char ch[2];
                ch[0] = 3; // Message ID
                ch[1] = 65;
                send_packet(ch, 2);
                printf("select music\n");
            }
            // Update the time
            last_rot = TIMERS_GetMilliSeconds();
        }    

        //printf("Piezo: %d Above: %d\n", piezo, piezo > 90);
        unsigned int piezo = ADC_Read(ADC_2) * 2; // This is reading the values from the piezo sensor
        if (piezo > 180) { // PIEZO is Touched
            // Trigger after debounce
            int delta = TIMERS_GetMilliSeconds() - last_piezo;
            if (delta > 500) {
                //printf("Toggle? %d\n", piezo);
                if (music_state == true) {
                    music_state = false;
                    char ch[2];
                    ch[0] = 6; // Play event
                    ch[1] = 67;
                    send_packet(ch, 2);
                    printf("Play...\n");
                } else {
                    music_state = true;
                    char ch[2];
                    ch[0] = 7; // Play event
                    ch[1] = 68;
                    send_packet(ch, 2);
                    printf("Pause...\n");
                }
                // Update the time
                last_piezo = TIMERS_GetMilliSeconds();
            }
        }
        
        if (music_state == true) {
            PWM_SetDutyCycle(PWM_1, 100);
            PWM_SetDutyCycle(PWM_3, 0);
            PWM_SetDutyCycle(PWM_5, 100);
        } else {
            PWM_SetDutyCycle(PWM_1, 65);
            PWM_SetDutyCycle(PWM_3, 100);
            PWM_SetDutyCycle(PWM_5, 0);
        }
    }
    BOARD_End();
    return 0;
}