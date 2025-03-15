#include <Board.h>
#include <stdint.h>
#include <QEI.h>
#include <stdio.h>
#include <stdbool.h>

// Static variables for encoder state and button press flag
static volatile int32_t encoder_count = 0;
static volatile int8_t last_state = 0;
static volatile bool buttonPressed = false;

/**
 * Reads the two input signals (ENC_A and ENC_B) from the quadrature encoder
 * and encodes their logic levels into a 2-bit integer.
 */
static int8_t QEI_ReadState(void) {
    int8_t state = 0;
    if (HAL_GPIO_ReadPin(GPIOB, ENC_A)) {
        state |= 0x01;
    }
    if (HAL_GPIO_ReadPin(GPIOB, ENC_B)) {
        state |= 0x02;
    }
    return state;
}

/**
 * Returns whether the button has been pressed since the last check.
 * The flag is cleared after reading so that a button press is only reported once.
 */
bool QEI_ButtonStatus(void) {
    bool pressed = buttonPressed;
    buttonPressed = false;  // Clear the flag after reading
    return pressed;
}

/**
 * Returns the current position of the Quadrature Encoder in degrees.
 */
int QEI_GetPosition(void) {
    return encoder_count / 4;
}

/**
 * Resets the encoder count to zero.
 */
void QEI_ResetPosition(void) {
    encoder_count = 0;
}

/**
 * Initializes the Quadrature Encoder and button.
 * Configures:
 *   - PB4 and PB5 for the encoder signals (with interrupts on rising and falling edges).
 *   - PA6 for the button (configured with interrupt on rising edge only).
 */
void QEI_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // Configure encoder pins: PB4 and PB5 with interrupts on rising and falling edges.
    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Configure button pin: PA6 as an interrupt source.
    // Using rising edge only (change to falling if your button is active-low)
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // NVIC configuration:
    // EXTI4 is used for PB4
    HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    // EXTI9_5 covers PB5 and PA6
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    
    encoder_count = 0;
    last_state = QEI_ReadState();
}

/**
 * Implements the state machine for handling encoder signals.
 * Determines the direction of rotation based on state transitions.
 */
void QEI_IRQ(void) {
    int8_t current_state = QEI_ReadState();

    // Clockwise rotation detection
    if ((last_state == 0x00 && current_state == 0x01) ||
        (last_state == 0x01 && current_state == 0x03) ||
        (last_state == 0x03 && current_state == 0x02) ||
        (last_state == 0x02 && current_state == 0x00)) {
        encoder_count++;
    }
    // Counter-clockwise rotation detection
    else if ((last_state == 0x00 && current_state == 0x02) ||
             (last_state == 0x02 && current_state == 0x03) ||
             (last_state == 0x03 && current_state == 0x01) ||
             (last_state == 0x01 && current_state == 0x00)) {
        encoder_count--;
    }
    last_state = current_state;
    
    // Reset encoder count if it exceeds the range -96 to 96
    if (encoder_count <= -96 || encoder_count >= 96) {
        encoder_count = 0;
    }
}

/**
 * EXTI9_5_IRQHandler handles interrupts for:
 *   - PB5 (encoder channel) by calling QEI_IRQ().
 *   - PA6 (button) by setting the buttonPressed flag.
 */
void EXTI9_5_IRQHandler(void) {
    // Handle encoder signal from PB5
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);
        QEI_IRQ();
    }
    // Handle button press from PA6
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);
        buttonPressed = true;
    }
}

/**
 * EXTI4_IRQHandler handles interrupts from PB4 (encoder channel).
 */
void EXTI4_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);
        QEI_IRQ();
    }
}
