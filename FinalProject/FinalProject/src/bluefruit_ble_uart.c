/*
 * File:   bluefruit_ble_uart.c
 * Author: Derrick Lai
 *
 * Library functions for setting up the Adafruit Bluefruit UART Friend
 * This library uses the UART6 to be used for the bluetooth chip.
 * 
 * Created on March 9, 2025
 */

/******************************************************************************
 * Libraries
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * User Libraries
 *****************************************************************************/
#include "Board.h"
#include "leds.h"
#include "timers.h"
#include "bluefruit_ble_uart.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#ifndef FALSE
#define FALSE ((int8_t) 0)
#endif

#ifndef TRUE
#define TRUE ((int8_t) 1)
#endif


#ifndef ERROR
#define ERROR ((int8_t) -1)
#endif

#ifndef SUCCESS
#define SUCCESS ((int8_t) 1)
#endif

#define BUFFER_SIZE 16
#define BLE_BAUD_RATE 9600 // Baud rate, always 9600 for Bluefruit

/******************************************************************************
 * Privates
 *****************************************************************************/
static uint8_t global_ble_uart_status = FALSE; // Initialization status of the BLE UART.

// Initially both TRUE so that buffers have time to fill. Except for RX, it will run as it will just wait for an incoming character.
static uint8_t is_tx_buffer_yielded = TRUE; // If the buffer is full, the callback won't continue to call the function to fill the buffer.
static uint8_t is_rx_buffer_yielded = TRUE;
static uint8_t tx_char, rx_char; // Used to hold the characters loaded from either the buffers or registers.

typedef struct CircularBuffer {
    char data[BUFFER_SIZE]; // Starting address of the buffer
    int tail; // Where to write to the buffer
    int head; // Where to read from the buffer
    int full; // Whether if the buffer is full.
    int empty; // Whether if the buffer is empty
}CircularBuffer;


struct CircularBuffer tx_buffer;
struct CircularBuffer rx_buffer;


uint8_t led_count = 0;
/******************************************************************************
 * Declarations
 *****************************************************************************/
uint8_t ReadFromBuffer(struct CircularBuffer *buffer, uint8_t *data);
uint8_t WriteToBuffer(struct CircularBuffer *buffer, uint8_t data);

/******************************************************************************
 * Main
 *****************************************************************************/
/**
 * @Function BLE_UART_Init(Rate)
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Initializes UART6 for the Bluefruit, baud rate will be set at a default 9600
 * @author Derrick Lai, 2025.03.09 */
int8_t BLE_UART_Init() {
    // If initialization has already been done, return SUCCESS
    if (global_ble_uart_status == TRUE) {
        return SUCCESS;
    }

    // Initialize the properties for the UART6
    // You have to set 'huart6'? Is this because the 'stm32f4xx_it.c' file declared this UART6?
    huart6.Instance = USART6;
    huart6.Init.BaudRate = BLE_BAUD_RATE;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_CTS; // CTS control is present for the Bluefruit
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;

    // Interrupts
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_RXNE); // Enable Receive interrupt
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_TC);   // Enable Transmit complete interrupt
    
    // Priorities
    HAL_NVIC_SetPriority(USART6_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART6_IRQn);


    // Error if initialization failure occurs
    if (HAL_UART_Init(&huart6) != HAL_OK) {
        return ERROR;
    }

    // Update new status
    global_ble_uart_status = TRUE;

    // Initialize the circular buffers
    // Transmit buffer
    tx_buffer.tail = 0;
    tx_buffer.head = 0;
    tx_buffer.empty = 1;
    tx_buffer.full = 0;

    // Receive buffer
    rx_buffer.tail = 0;
    rx_buffer.head = 0;
    rx_buffer.empty = 1;
    rx_buffer.full = 0;

    // Call the initial transmit and receiver functions
    // HAL_UART_Transmit_IT(&huart6, &tx_char, 1);
    // HAL_UART_Receive_IT(&huart6, &rx_char, 1);

    return SUCCESS;
}

// PutChar for sending packets (transmissions), GetChar for receiving packets for state machine.
/**
 * @Function BLE_GetChar()
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Reads from the RX circular buffer, returns a character if there exist one.
 * @author Derrick Lai, 2025.03.09 */
int8_t BLE_GetChar(unsigned char* data) {

    // If the RX buffer is empty, then return ERROR as nothing can be retrieved.
    if (rx_buffer.empty) {
        return ERROR;
    }

    // If the data is null, return ERROR
    if (data == NULL) {
        return ERROR;
    }

    // Read from the RX buffer
    ReadFromBuffer(&rx_buffer, data);
    return SUCCESS;
}

/**
 * @Function BLE_PutChar()
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Writes to the TX circular buffer, for putting together packets
 * @author Derrick Lai, 2025.03.09 */
int8_t BLE_PutChar(uint8_t data) {

    // If the TX buffer is full, then return error
    if (tx_buffer.full) {
        return ERROR;
    }

    // Put the character into the transmit buffer
    WriteToBuffer(&tx_buffer, data);


    return SUCCESS;
}

/**
 * @Function BLE_RunLoop()
 * @param None
 * @return None
 * @brief  This function should continously runs in a while loop every tick. Necessary to keep the UART running. DO NOT APPLY DELAY, IT MUST RUN CONTINOUSLY.
 * @author Derrick Lai, 2025.03.09 */
void BLE_RunLoop() {

    // If the transmit yield flag is raised, then check if the buffer is still empty. If it isn't, unraise the flag and begin a transmission.
    if (is_tx_buffer_yielded && !tx_buffer.empty) {
        is_tx_buffer_yielded = FALSE;
        ReadFromBuffer(&tx_buffer, &tx_char);
        HAL_UART_Transmit_IT(&huart6, &tx_char, 1);
    }

    // If the receive yield flag is raised, then check to see if the buffer is still full, if it isn't then unraise the flag and begin the reception process.
    if (is_rx_buffer_yielded && !rx_buffer.full) {
        is_rx_buffer_yielded = FALSE;
        HAL_UART_Receive_IT(&huart6, &rx_char, 1);
    }
}

 /******************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * @Function ReadFromBuffer()
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Reads a character from the buffer
 * @author Derrick Lai, 2025.03.09 */
uint8_t ReadFromBuffer(struct CircularBuffer *buffer, uint8_t *data) {

    // If the buffer is empty, then nothing can be read, return ERROR
    if (buffer->empty) {
        return ERROR;
    }

    // Otherwise, read the value from the buffer and set to the data pointer.
    *data = buffer->data[buffer->head];

    // Increment the head and update new value
    buffer->head = (buffer->head + 1) % BUFFER_SIZE;

    // If the buffer is now empty (head and tail equal), set the status
    buffer->empty = (buffer->head == buffer->tail);

    // Set buffer full status to 0 (you can't read from an empty buffer)
    buffer->full = 0;

    //  Return status
    return SUCCESS;
}

/**
 * @Function WriteToBuffer()
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Reads a character from the buffer
 * @author Derrick Lai, 2025.03.09 */
uint8_t WriteToBuffer(struct CircularBuffer *buffer, uint8_t data) {

    // If the buffer is full, then return an ERROR
    if (buffer->full) {
        return ERROR;
    }
    
    // Write to the tail
    buffer->data[buffer->tail] = data;

    // Increment the tail
    buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;

    // Check if the buffer is full (if head and tail are equal, which means we've circled back).
    buffer->full = (buffer->head == buffer->tail);

    // Set buffer empty status to 0 (you can't be empty after just writing to it)
    buffer->empty = 0;

    // Return status
    return SUCCESS;
}

 /******************************************************************************
 * Interrupts
 *****************************************************************************/
// https://www.st.com/content/ccc/resource/technical/document/user_manual/a6/79/73/ae/6e/1c/44/14/DM00122016.pdf/files/DM00122016.pdf/jcr:content/translations/en.DM00122016.pdf
// Page 34 of 1354
// This part is automatically handled by stm32f4xx_it.h in Common -> Framework Files
// void USART6_IRQHandler(void) {
//     // Whenever an interrupt occurs, handle the interrupt (it will clear the flags and call the necessary callbacks)
//     HAL_UART_IRQHandler(&huart6);
// }

// This callback is triggered when a character has been successfully transmitted by a HAL_UART_Transmit_IT() call.
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {

    // Only run the callback if it applies for the UART6
    if (huart->Instance != USART6) {
        return;
    }

    // After transmitting a character, we want to keep transmitting until the tx_buffer has nothing left to transmit.
    // If the buffer is empty, there are no characters to transmit, we have to wait until there is.
    if (tx_buffer.empty) {
        is_tx_buffer_yielded = TRUE;

    } else {
        // If the tx_buffer is not empty, then read from it, and transfer that character.
        ReadFromBuffer(&tx_buffer, &tx_char);
        HAL_UART_Transmit_IT(&huart6, &tx_char, 1); // Send the characater as a pointer, the size is just 1 byte.
        //led_count++;
    }
}

// This callback is triggered when a character has been successfully received by a HAL_UART_Receive_IT() call.
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

    // Only run the callback if it applies for the UART6
    if (huart->Instance != USART6) {
        return;
    }

    // After a character has been received, we want to place it into the receive buffer to future processing.
    WriteToBuffer(&rx_buffer, rx_char);

    // If the receive buffer is full after placing that character, we will have to take in characters later, raise the flag to do so.
    if (rx_buffer.full) {
        is_rx_buffer_yielded = TRUE;
    } else {
        // Otherwise, if there is still space, continue the process by reading a character from the receive buffer.
        HAL_UART_Receive_IT(&huart6, &rx_char, 1); // I think since it assumes the message is an array, we have to convert the unsigned char to a pointer.
    }
}

/*
HAL_UART_Transmit_IT(UART6, Message To Send, Message Size) -> This sends the message into the TRANSMIT REGISTER, after the entire message is sent TxCplt Callback is called.
HAL_UART_Receive_IT(UART6, Message Holder, Messasge Size) -> This takes bytes from the RECEIVE REGISTER and fills up the Message Holder for however long Message Size is.
*/

/******************************************************************************
 * Testing
 *****************************************************************************/
//#define BLE_UART_TEST
#ifdef BLE_UART_TEST
int main() {

    // Initialization
    BOARD_Init();
    TIMER_Init();
    LEDS_Init();

    uint8_t ble_status = BLE_UART_Init();
    if (ble_status == ERROR) {
        set_leds(0xFF);
    }

    // // Populate the transmit buffer
    // uint8_t ch = 'A';
    // for (int i = 0; i < (BUFFER_SIZE); i++) {
    //     BLE_PutChar(ch + i + 6);
    //     //set_leds(tx_buffer.tail);
    //     printf("Val: %d \n", tx_buffer.tail);
    // }

    
    while (TRUE) {
        BLE_RunLoop();
        //set_leds(tx_buffer.tail);

        // If a character is present, print it out...?
        uint8_t x;
        uint8_t status = BLE_GetChar(&x);
        if (status == SUCCESS) {
            BLE_PutChar(x); // Loopback test, put the character and send it.
            printf("Msg: %c\n", x); // UART is set to have just a newline ending, so don't need to put it there, '\n'
        }
    }

    // while (TRUE) {

    //     // Peridiocally insert a character for transmission
    //     int current = TIMERS_GetMilliSeconds();
    //     int delta = current - last_time;
    //     if (delta >= 1000) {
    //         // Put a character in

    //         // The PutChar is causing some issues, the code must have stopped execution here...
    //         set_leds(0x01);
    //         uint8_t ch = 'A';
    //         uint8_t put_status = BLE_PutChar(ch);
    //         set_leds(0x02);
    //         //set_leds(led_count);
    //         //led_count++;

    //         last_time = current;
    //     }

    //     //set_leds(led_count);

    //     // Run the loop (this should always be running)
    //     BLE_RunLoop();
    // }

    return 1;
}

#endif