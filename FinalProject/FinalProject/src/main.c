#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <pwm.h>
#include <timers.h>
#include <QEI.h>
#include <ADC.h>

#include <CAPTOUCH1.h>
#include <CAPTOUCH2.h>

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

nextSong = 0;
prevSong = 0;
Music_status = 0; // 0 is pause, 1 is play
selectMusic = 0;
Scroll_Up = 0;
Scroll_Down = 0;

int main(void) {
    // Initialize all modules
    PWM_Init();
    BOARD_Init();
    CAPTOUCH1_Init();
    CAPTOUCH2_Init();
    QEI_Init();
    TIMER_Init();
    ADC_Init();

    while (TRUE) {

        unsigned int piezo = ADC_Read(ADC_2) * 2; // This is reading the values from the piezo sensor
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

        if (CAPTOUCH1_IsTouched()) { // Next Song
            nextSong = 1;
            return;
        } 
        if (CAPTOUCH2_IsTouched()){ // Previous Song
            prevSong = 1;
            return;
        }
        if (rot > 0){ // Scrolling up
            Scroll_Up = 1;
            return;
        } else if (rot < 0){ // Scrolling Down
            Scroll_Down = 1; 
            return;
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
            return;
        }    
    }

    BOARD_End();
    return 0;
}
