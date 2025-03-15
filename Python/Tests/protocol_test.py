"""
protocol_test.py
Author: Derrick Lai
Date: 2025-03-14
Description: This program is meant to run test cases for protocol.py.
"""
# =============================================
#                   IMPORTS
# =============================================
import sys
import os
import threading
import time

# Get the main directory
parent_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, parent_dir)
from protocol import Protocol

# =============================================
#                   CONSTANTS
# =============================================
ADAFRUIT_BLE_MAC_ADDR = "C0:9E:48:AC:35:03"
MAX_QUEUE_SIZE= 16

# =============================================
#                     MAIN
# =============================================
def compute_iterative_checksum(char_byte : int, previous_chk_sum : str):
        """
        @name: __compute_iterative_checksum
        @param new_char: The new character to compute the new checksum on.
        @param: previous_chk_sum: The previously calculated checksum.
        @return: None
        @brief: This calculates the checksum of the payload data, whenever a value belonging to the payload is sent, its checksum
        must be calculated to validate the data.
        """
        # The initial value of the checksum is the previous value.
        checksum = previous_chk_sum
        
        # Perform circular rotation, then add the bit value of the new character to it (use ord() to convert from char to byte)
        checksum = (checksum >> 1) + (checksum << 7)
        checksum += char_byte
        
        # We only want the bottom 8 bits, so bitmask and with 1111 1111 -> 0xFF
        checksum &= 0xFF
        
        return checksum
    
def compute_iterative_checksum_old(new_char : str, previous_chk_sum : str):
        """
        @name: __compute_iterative_checksum
        @param new_char: The new character to compute the new checksum on.
        @param: previous_chk_sum: The previously calculated checksum.
        @return: None
        @brief: This calculates the checksum of the payload data, whenever a value belonging to the payload is sent, its checksum
        must be calculated to validate the data.
        """
        # The initial value of the checksum is the previous value.
        checksum = previous_chk_sum
        
        # Perform circular rotation, then add the bit value of the new character to it (use ord() to convert from char to byte)
        checksum = (checksum >> 1) + (checksum << 7)
        checksum += ord(new_char)
        
        # We only want the bottom 8 bits, so bitmask and with 1111 1111 -> 0xFF
        checksum &= 0xFF
        
        return checksum

    
def checksum_test_1():
    
    # # 0x817F → 0x3F
    # hex1 = '10000001' # 0x81
    # hex2 = '01111111' # 0x7F

    # # Convert the next to its char form. From value to base 2 int
    # char1 = chr(int(hex1, 2))
    # char2 = chr(int(hex2, 2))

    # checksum = compute_iterative_checksum_old(char1, 0)
    # print(hex(checksum))

    # checksum = compute_iterative_checksum_old(char2, checksum)
    # print(hex(checksum))

    # print('=====')
    # print(bin(int("80", 16)))

    # # 0x807F35 → 0x14
    # # Ta
    # ch1 = chr(int("80", 16))
    # ch2 = chr(int("7F", 16))
    # ch3 = chr(int("35", 16))

    # checksum = 0
    # checksum = compute_iterative_checksum_old(ch1, checksum)
    # print(hex(checksum))

    # checksum = compute_iterative_checksum_old(ch2, checksum)
    # print(hex(checksum))

    # checksum = compute_iterative_checksum_old(ch3, checksum)
    # print(hex(checksum))
    
    # 0x8400257D96 → 0xE6
    ch1 = chr(int("84", 16))
    ch2 = chr(int("00", 16))
    ch3 = chr(int("25", 16))
    ch4 = chr(int("7D", 16))
    ch5 = chr(int("96", 16))
    
    checksum = 0
    checksum = compute_iterative_checksum_old(ch1, checksum)
    print(hex(checksum))

    checksum = compute_iterative_checksum_old(ch2, checksum)
    print(hex(checksum))

    checksum = compute_iterative_checksum_old(ch3, checksum)
    print(hex(checksum))
    
    checksum = compute_iterative_checksum_old(ch4, checksum)
    print(hex(checksum))

    checksum = compute_iterative_checksum_old(ch5, checksum)
    print(hex(checksum))
    print(checksum)
    
    print(int("84", 16))
    print(int("00", 16))
    print(int("25", 16))
    print(int("7D", 16))
    print(int("96", 16))

def checksum_test2():
    
    checksum = 0
    checksum = compute_iterative_checksum(132, checksum)
    checksum = compute_iterative_checksum(0, checksum)
    checksum = compute_iterative_checksum(37, checksum)
    checksum = compute_iterative_checksum(125, checksum)
    checksum = compute_iterative_checksum(150, checksum)
    print(f"Checksum: {checksum}")

def find_checksum():
    # 0x00 0x00 0x25 0x7D 0x96
    # [0, 0, 37, 125, 150]
    checksum = 0
    checksum = compute_iterative_checksum(0, checksum)
    checksum = compute_iterative_checksum(0, checksum)
    checksum = compute_iterative_checksum(37, checksum)
    checksum = compute_iterative_checksum(125, checksum)
    checksum = compute_iterative_checksum(150, checksum)
    print(f"Checksum: {checksum}")
    pass

def main():
    print("Running Protocol Test")
    protocol = Protocol(ADAFRUIT_BLE_MAC_ADDR, MAX_QUEUE_SIZE)

    # Keep waiting for a packet (Reception Test) -> Note: The packets are all in integer form.
    while True:
        packet = protocol.get_packet()
        
        if packet:
            print(packet)
    
    # Send packets (Transmission Test)
    # Data: [204, 5, [132, 0, 37, 125, 150], 230, 185]
    # data = [104, 101, 108, 108, 111] # Prints out hello
    # while True:
    #     # Send a packet
    #     # print("Transmitting...")
    #     protocol.send_packet(data)
        
    #     # Delay a second
    #     time.sleep(1)
        
    
    # Block the program from exiting
    exit_event = threading.Event()
    try:
        exit_event.wait()
    except KeyboardInterrupt:
        print("Protocol test program terminated.")

# =============================================
#                    DRIVER
# =============================================
if __name__ == "__main__":
    main()