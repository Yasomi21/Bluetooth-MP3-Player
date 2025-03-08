#include <Board.h>
#include <stdint.h>
#include <QEI.h>
#include <stdio.h>

// Static variables for encoder state
static volatile int32_t encoder_count = 0;
static volatile int8_t last_state = 0;
/**
 * This function reads the two input signals (ENC_A and ENC_B) from the quadrature encoder
 * and encodes their logic levels into a 2-bit integer. Each bit represents the state of one
 * encoder pin: bit 0 for ENC_A and bit 1 for ENC_B. The returned value is used in the
 * state machine to determine the direction of rotation.
 */
static int8_t QEI_ReadState(void) {
    int8_t state = 0;
      // Check the state of ENC_A pin and set bit 0 if it is HIGH
    if (HAL_GPIO_ReadPin(GPIOB, ENC_A)) {
        // set bit to 0
        state |= 0x01;
    }
     // Check the state of ENC_B pin and set bit 1 if it is HIGH
    if (HAL_GPIO_ReadPin(GPIOB, ENC_B)) {
        //set bit to 1
        state |= 0x02;
    }
    return state;
}

//int count = 0;


/**
 * @function QEI_GetPosition(void) 
 * @param none
 * @brief This function returns the current position of the Quadrature Encoder in degrees.      
*/
int QEI_GetPosition(void){
    return encoder_count/4;
}

/**
 * @Function QEI_ResetPosition(void) 
 * @param  none
 * @return none
 * @brief  Resets the encoder such that it starts counting from 0.
*/
void QEI_ResetPosition(){
    encoder_count = 0;
} 


/**
 * @function QEI_Init(void)
 * @param none
 * @brief  Enables the external GPIO interrupt, anything
 *         else that needs to be done to initialize the module. 
 * @return none
*/
void QEI_Init(void){
     //Configure GPIO pins : PB4 PB5 
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    encoder_count = 0;
    last_state = QEI_ReadState();
}
/**
 * This function implements the state machine for quadrature encoder signals.
 * It determines the direction of rotation and updates the encoder count based on the
 * state transitions of the encoder's two channels (ENC_A and ENC_B). The encoder count
 * is incremented for forward rotation and decremented for reverse rotation. The count
 * is constrained within a range of -96 to +96, resetting to zero when the limits are exceeded.
 * 
 * 2. **State Transitions for Forward Rotation:**
 *    - 00 → 01 → 11 → 10 → 00 (Clockwise movement).
 *    - Each  transition increments the encoder count by 1.
 * 
 * 3. **State Transitions for Reverse Rotation:**
 *    - 00 → 10 → 11 → 01 → 00 (Counter clockwise movement).
 *    - Each  transition decrements the encoder count by 1.
 */
void QEI_IRQ() {
    //state machine of your design
    int8_t current_state = QEI_ReadState();
    //printf("state: %x|, count: %ld\n", current_state, encoder_count);

    // Determine the direction of rotation using state transitions
    if ((last_state == 0x00 && current_state == 0x01) ||
        (last_state == 0x01 && current_state == 0x03) ||
        (last_state == 0x03 && current_state == 0x02) ||
        (last_state == 0x02 && current_state == 0x00)) {
        encoder_count++;
    } else if ((last_state == 0x00 && current_state == 0x02) ||
               (last_state == 0x02 && current_state == 0x03) ||
               (last_state == 0x03 && current_state == 0x01) ||
               (last_state == 0x01 && current_state == 0x00)) {
        encoder_count--;
    }

    last_state = current_state;
    //the encoder count within the range -96 to 96
    if(encoder_count <= -96){
        encoder_count = 0;
    }
    if(encoder_count >= 96){
        encoder_count = 0;
    }
    
}


 // external interrupt ISR for pin PB5 
void EXTI9_5_IRQHandler(void) {
    // EXTI line interrupt detected 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5); // clear interrupt flag
        QEI_IRQ();
    }
}

// external interrupt ISR for pin PB4
void EXTI4_IRQHandler(void) {
    // EXTI line interrupt detected 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4); // clear interrupt flag
        QEI_IRQ();
    }
}



