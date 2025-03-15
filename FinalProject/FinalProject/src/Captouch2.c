#include <Board.h>
#include <timers.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include <stm32f4xx_hal_tim.h>

#include <stdio.h>
#include <stdlib.h>
#include <CAPTOUCH2.h>

// Defines //
#define BUFFER_SIZE 10 // The size of the buffer/moving average size.
#define CAP_THRESHOLD 60 // Experimentally determined threhsold.

// Globals //
char first_edge = TRUE;
int last_edge = 0;
int current_edge = 0;
int circular_buffer[BUFFER_SIZE] = {0}; // Create an nth size buffer with initial values of 0.
unsigned int idx = 0; // Read/Write index of the buffer.
unsigned int sum = 0; // Sum of the buffer.
unsigned int average = 0; // Average of the buffer values.

static unsigned int state = FALSE;

/*  PROTOTYPES  */
/** CAPTOUCH_Init()
 *
 * This function initializes the module for use. Initialization is done by
 * opening and configuring timer 2, opening and configuring the GPIO pin and
 * setting up the interrupt.
 */
void CAPTOUCH2_Init(void) {
    //Configure GPIO pin PB5 
    GPIO_InitTypeDef GPIO_InitStruct2 = {0};
    GPIO_InitStruct2.Pin = GPIO_PIN_2;
    GPIO_InitStruct2.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct2.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct2);

    // EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);

    // the rest of the function goes here
    TIMER_Init(); // Initialize the timer
}

/** CAPTOUCH_IsTouched(void)
 *
 * Returns TRUE if finger is detected. Averaging of previous measurements may
 * occur within this function, however you are NOT allowed to do any I/O inside
 * this function.
 *
 * @return  (char)    [TRUE, FALSE]
 */
char CAPTOUCH2_IsTouched(void) {
    
    // Debugging (print on the same line and then flush out everything to restart this process):
    // This is used to find the average value of the capacitance.
    //printf("\r\nAverage22222: %d     ", average);
   
    //return FALSE;

    unsigned int above_thr = (average >= CAP_THRESHOLD);
    if (above_thr && (state == FALSE)) {
        state = TRUE;
    } else if ((!above_thr) && (state == TRUE)) {
        state = FALSE;
    }

    // If the moving average exceeds the threshold, then return TRUE
    // return (average >= CAP_THRESHOLD);
    return state;
}


// Interrupts
// external interrupt ISR for rising edge of pin PB5
/*
Assuming this is an Input Capture interrupt, where it only triggers whenever a rising edge is detected.
Without knowing the behavior, not sure.
But assume to update with the time pressed since last and in the IsTouched function average out the previous measurements.
*/
void EXTI2_IRQHandler(void) {
    // EXTI line interrupt detected 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2) != RESET) {
        // Clear interrupt flag.
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);

        // Anything that needs to happen on rising edge of PB5
        // (ENC_B).

        // Whenever the rising edge is triggered.
        // If interrupt is triggered for the first time, then prevent trigger by setting current and previous to zero.
        // Otherwise, update the previous time to current and current to the time now in microseconds.
        if (first_edge) {
            last_edge = TIMERS_GetMicroSeconds();
            first_edge = TIMERS_GetMicroSeconds();
            first_edge = FALSE; // Prevent this from triggering again.
        } else {
            last_edge = current_edge;
            current_edge = TIMERS_GetMicroSeconds();
        }

        // Subtract the value you are overwriting from the sum before overwriting.
        sum -= circular_buffer[idx];

        // Write the delta value into the buffer.
        unsigned int delta = current_edge - last_edge; // Difference between rising edges to get estimated touch time.
        circular_buffer[idx] = delta;

        // Add the delta to the sum to get the new sum
        // This is more efficient than going through the buffer to get the sum.
        sum += delta;

        // Increment the circular buffer
        idx = (idx + 1) % BUFFER_SIZE;

        // Calculate and update the moving average.
        average = sum / BUFFER_SIZE;
    }
}
