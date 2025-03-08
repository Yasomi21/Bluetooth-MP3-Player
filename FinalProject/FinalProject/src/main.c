#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <pwm.h>
#include <timers.h>
//#include <PING.h>

#include <CAPTOUCH1.h>
#include <CAPTOUCH2.h>

#define DELAY(x)    {int wait; for (wait = 0; wait <= x; wait++) {asm("nop");}}
#define A_BIT       18300
#define A_LOT       183000

int glob_time_micro = 0;


int main(void) {
    // Initialize all modules
   
    //PWM_Init();
    BOARD_Init();
    CAPTOUCH1_Init();
    CAPTOUCH2_Init();
    //TIMER_Init();

   

    while (TRUE) {
        //char first_touch = CAPTOUCH1_IsTouched();
        char second_touch = CAPTOUCH2_IsTouched();
        //  if (first_touch){
        //     printf("touch!\n");
        //  }else{
        //     printf("nottouch!\n");
        //  }
    
  
        
    }

    BOARD_End();
    return 0;
}
