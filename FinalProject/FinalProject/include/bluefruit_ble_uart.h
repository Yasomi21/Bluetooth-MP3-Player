/*
 * File:   bluefruit_ble_uart.h
 * Author: Derrick Lai
 *
 * Library functions for setting up the Adafruit Bluefruit UART Friend
 * This library uses the UART6 to be used for the bluetooth chip.
 * 
 * Created on March 9, 2025
 */

#ifndef BLUEFRUIT_BLE_UART_H
#define BLUEFRUIT_BLE_UART_H

/******************************************************************************
 * Libraries
 *****************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
UART_HandleTypeDef huart6; // The UART6 for the Bluetooth Low Energy


/******************************************************************************
 * Functions
 *****************************************************************************/
/**
 * @Function BLE_UART_Init(Rate)
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Initializes UART6 for the Bluefruit, baud rate will be set at a default 9600
 * @author Derrick Lai, 2025.03.09 */
int8_t BLE_UART_Init();

// PutChar for sending packets (transmissions), GetChar for receiving packets for state machine.
/**
 * @Function BLE_ReadChar()
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Reads from the circular buffer, returns a character if there exist one.
 * @author Derrick Lai, 2025.03.09 */
int8_t BLE_GetChar(unsigned char* data);

/**
 * @Function BLE_SendChar()
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Writes to the circular buffer, for putting together packets
 * @author Derrick Lai, 2025.03.09 */
int8_t BLE_PutChar(uint8_t data);

/**
 * @Function BLE_RunLoop()
 * @param None
 * @return None
 * @brief  This function should continously runs in a while loop every tick. Necessary to keep the UART running. DO NOT APPLY DELAY, IT MUST RUN CONTINOUSLY.
 * @author Derrick Lai, 2025.03.09 */
void BLE_RunLoop();



#endif