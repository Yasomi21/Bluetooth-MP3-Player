#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <timers.h>
#include "stm32f4xx_hal.h"
#include <stm32f4xx_hal_tim.h>
#include <stdbool.h>
#include <CAPTOUCH2.h>
#include <buttons.h>

// Defines for capacitive touch
#define BUFFER_SIZE    10    // Size of the moving average buffer
#define CAP_THRESHOLD  50    // Experimentally determined threshold

// Global variables for capacitive touch measurement
char first_edge = TRUE;
int last_edge = 0;
int current_edge = 0;
int circular_buffer[BUFFER_SIZE] = {0};
unsigned int idx = 0;
unsigned int sum = 0;
unsigned int average = 0;
static unsigned int touchState = FALSE; // TRUE if touch detected

// Timer3 handle for periodic processing
TIM_HandleTypeDef htim3;

// Define a state machine for the timer interrupt
typedef enum {
    TOUCH_CHECK,  // Regularly check if the sensor is touched
    TOUCH_RESET   // Wait for the sensor to be released
} TouchTimerState;

static TouchTimerState currentTouchState = TOUCH_CHECK;

/*--------------------------------------------------------------
    Function Prototypes
--------------------------------------------------------------*/
void CAPTOUCH2_Init(void);
char CAPTOUCH2_IsTouched(void);
void Timer3_Init(void);

/*--------------------------------------------------------------
    CAPTOUCH2 Initialization
    - Configures the sensor input pin (using PD2 in this example)
    - Sets up the external interrupt and the timer for measurement.
--------------------------------------------------------------*/
void CAPTOUCH2_Init(void) {
    // Initialize the board and the timer used for microsecond timing.
    BOARD_Init();
    TIMER_Init();
    
    // Configure sensor input on PD2 (adjust as needed for your board)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    // Using rising edge interrupt; if needed, you could also detect falling edges.
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Configure NVIC for the sensor interrupt
    HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);

    // Initialize Timer3 for periodic sampling of the capacitive touch sensor.
    Timer3_Init();
}

/*--------------------------------------------------------------
    CAPTOUCH2_IsTouched()
    - Uses the moving average of the raw measurement (updated in EXTI2 IRQ)
      and compares it to a threshold.
--------------------------------------------------------------*/
char CAPTOUCH2_IsTouched(void) {
    
    if (average >= CAP_THRESHOLD)
        touchState = TRUE;
    else
        touchState = FALSE;
    
    return touchState;
}

/*--------------------------------------------------------------
    External Interrupt Handler for Sensor Input (PD2)
    - Each rising edge is used to compute the time delta (in microseconds)
      between events. A moving average is computed by using a circular buffer.
--------------------------------------------------------------*/
void EXTI2_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);
        
        if (first_edge) {
            last_edge = TIMERS_GetMicroSeconds();
            current_edge = TIMERS_GetMicroSeconds();
            first_edge = FALSE;
        } else {
            last_edge = current_edge;
            current_edge = TIMERS_GetMicroSeconds();
        }
        
        // Update the moving average buffer:
        sum -= circular_buffer[idx];
        unsigned int delta = current_edge - last_edge;
        circular_buffer[idx] = delta;
        sum += delta;
        idx = (idx + 1) % BUFFER_SIZE;
        average = sum / BUFFER_SIZE;
    }
}

/*--------------------------------------------------------------
    Timer3 Initialization
    - Configures Timer3 to trigger an interrupt at a fixed period.
    - In this example, we use a 10 ms period (adjust prescaler/period as needed).
--------------------------------------------------------------*/
void Timer3_Init(void) {
    __HAL_RCC_TIM3_CLK_ENABLE();
    
    // For example, assume the system clock is 84MHz.
    // With a prescaler of 8400-1, the timer clock is 10 kHz.
    // Setting Period to 100-1 gives a period of 100/10,000 = 10ms.
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 8400 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 100 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        // Handle error appropriately (e.g., loop indefinitely)
        while (1);
    }
    
    // Configure NVIC for Timer3 interrupts
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    
    // Start Timer3 in interrupt mode
    HAL_TIM_Base_Start_IT(&htim3);
}

/*--------------------------------------------------------------
    Timer3 Interrupt Handler
--------------------------------------------------------------*/
void TIM3_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim3);
}

