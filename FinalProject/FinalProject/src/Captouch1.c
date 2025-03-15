#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Board.h>
#include <CAPTOUCH1.h>
#include <timers.h>
#define NUM_SAMPLES 10  // Number of samples for averaging
#define THRESH_HOLE 60

char is_first_interrupt = TRUE;
unsigned int previous_time = 0;
unsigned int current_time = 0;
unsigned int time_differences[NUM_SAMPLES] = {0};
unsigned int buffer_index = 0;
unsigned int total_time = 0;
unsigned int average_time = 0;

static unsigned int state = FALSE;

void CAPTOUCH1_Init(void) {
    // Configure GPIO pin PB5 
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 3);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    // Initialize TIMER
    TIMER_Init();
}

char CAPTOUCH1_IsTouched(void) {
    //printf("\r\nAverage1111: %d     ", average_time);
    //return FALSE;

    unsigned int above_thr = (average_time >= THRESH_HOLE);
    if (above_thr && (state == FALSE)) {
        state = TRUE;
    } else if ((!above_thr) && (state == TRUE)) {
        state = FALSE;
    }


    return state;

    //return (average_time >= THRESH_HOLE);
}
/*
 * This function handles the interrupt triggered by a rising edge on GPIO Pin 5. 
 * It calculates the time difference between consecutive interrupts and maintains 
 * a circular buffer to compute a running average of these time intervals. 
 * This average is used to detect patterns, such as touches on a capacitive sensor.
 */
void EXTI15_10_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);  // Clear interrupt flag
        // skipping the first processing of interrupt, the previous_time is initialized for subsequent calculations.
        if (is_first_interrupt) {
            previous_time = TIMERS_GetMicroSeconds();
            is_first_interrupt = FALSE;
            return; 
        }

        current_time = TIMERS_GetMicroSeconds();

        // Calculate time difference 
        unsigned int time_difference = current_time - previous_time;
        previous_time = current_time; // Update previous time

        // Update circular buffer and total time
        total_time -= time_differences[buffer_index];  // Remove the oldest value
        time_differences[buffer_index] = time_difference; // Store new time difference
        total_time += time_difference;  // Add new value to total
        // Increment buffer index with wrap-around
        buffer_index = (buffer_index + 1) % NUM_SAMPLES;
        // Calculate new average
        average_time = total_time / NUM_SAMPLES;   
    }
}
