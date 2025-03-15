"""
event_test.py
Author: Derrick Lai
Date: 2025-03-15
Description: This program is used to test out the event.
"""
# =============================================
#                   IMPORTS
# =============================================
import sys
import os
import asyncio
import threading
from enum import Enum

# Get the main directory
parent_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, parent_dir)

from event_handler import EventHandler
from events import Events

# =============================================
#                   CONSTANTS
# =============================================
ADAFRUIT_BLE_MAC_ADDR = "C0:9E:48:AC:35:03"
MAX_PACKET_QUEUE_SIZE = 16

# =============================================
#                     MAIN
# =============================================
def event_cb(payload):
    print("Event triggered", payload)
    
def main():
    # Create the event handler
    event_handler = EventHandler(ADAFRUIT_BLE_MAC_ADDR, MAX_PACKET_QUEUE_SIZE)
    
    # Setup event
    event_handler.on_event(Events.EXAMPLE_EVENT, event_cb)
    
    # Run the event loop
    event_handler.run_event_loop()
    # Set event loop to prevent termination
    # exit_event = threading.Event()
    # try:
    #     exit_event.wait()
    # except KeyboardInterrupt:
    #     print("Terminating event_test.py")

# =============================================
#                    DRIVER
# =============================================
if __name__ == "__main__":
    main()