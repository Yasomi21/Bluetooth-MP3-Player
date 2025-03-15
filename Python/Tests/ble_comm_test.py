"""
ble_comm_test.py
Author: Derrick Lai
Date: 2025-03-11
Description: This program is meant to run test cases for ble_comm.py.
"""
# =============================================
#                   IMPORTS
# =============================================
import sys
import os
import threading

# Get the main directory
parent_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, parent_dir)
from ble_comm import BluefruitComm

# =============================================
#                   CONSTANTS
# =============================================

# =============================================
#                     MAIN
# =============================================
def main():
    # Create a new instance of the BluefruitComm
    ADAFRUIT_BLE_MAC_ADDR = "C0:9E:48:AC:35:03"
    bf_comm = BluefruitComm(ADAFRUIT_BLE_MAC_ADDR)
    print("Hello World!")
    
    # Bad practice, but for testing purposes. Keep reading from the buffer
    while True:
        # Loopback test, after receiving a message from the queue, send it back
        # Obtain the message from the queue
        msg = bf_comm.get_message()
        if msg is None:
            continue
        
        # Send the message back
        print(f"The message is: {chr(msg)}")
        bf_comm.send_message("Hello")
    
    # Warning: If we reach the end of the main program, it will terminate unless you prevent it.
    # Notice how GUI libraries have a set_loop() function at the end, those are use for this exact reason.
    # Prevent termination: Don't let the thread terminate until a Keyboard Interrupt is raised.
    # exit_event only stops if exit_event.set() is called.
    exit_event = threading.Event()
    try:
        exit_event.wait()
    except KeyboardInterrupt:
        print("Terminated test program.")
    
    

# =============================================
#                    DRIVER
# =============================================
if __name__ == "__main__":
    main()