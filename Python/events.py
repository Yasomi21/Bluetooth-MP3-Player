"""
events.py
Author: Derrick Lai
Date: 2025-03-15
Description: This script is used to set custom events needed by the user for their main interface.

You can add an event by adding a enum to the class 'Events' along with the integer value.
This integer value represents the message ID you will be sending from your STM32.

Note: Because of the limitations from the MessageID protocol, you can only create 256 events. ( Absolutely devastating :| )
This is because the MessageID is only a single byte or 8 bits.

NOTE: WHATEVER YOU DO, DON'T ADD ANY COMMAS ONTO THE ENUMS, JUST LEAVE THE NUMBERS AS IS.

IMPORTANT NOTE:
DON'T CHANGE ANYTHING IN THE FOLLOWING FILES
- ble_comm.py
- protocol.py
- event_handler.py

Here is an example as to how to use the events.
Let's say you want to sent 'EXAMPLE_EVENT' to the main interface.

1) Configure the STM32 to send the packet. Set the MessageID byte of your packet to 0
The value of the enum is the value of the MessageID

2) Whenever you send a packet, it should trigger a event, which you can process through the main interface.

Example:
main_interface.py
'''
from event_handler import EventHandler
from events import Events

# CONSTANTS
MAC_ADDRESS = "XX:XX:XX:XX:XX:XX"
MAX_BUFFER_SIZE = 16

# Callback event for when the event triggers
def event_callback(payload):
    for byte in payload:
        print(f"Data from the payload (sent as an integer): {str(byte)}")

# Set up the event
ev_handler = EventHandler(MAC_ADDRESS, MAX_BUFFER_SIZE)
ev.on_event(Events.EXAMPLE_EVENT, event_callback)

# Keeps the program alive
ev.run_event_loop()
'''
"""
# =============================================
#                   IMPORTS
# =============================================
from enum import Enum

# =============================================
#                   CLASSES
# =============================================
class Events(Enum):
    # Example
    EXAMPLE_EVENT = 0
    
    # Music selection events
    MUSIC_SELECT_LEFT = 1
    MUSIC_SELECT_RIGHT = 2
    MUSIC_SELECT = 3
    
    # Song skipping
    SONG_SKIP_PREV = 4
    SONG_SKIP_NEXT = 5
    
    # Playing/pausing sounds
    SONG_PLAY = 6
    SONG_PAUSE = 7