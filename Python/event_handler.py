"""
event_handler.py
Author: Derrick Lai
Date: 2025-03-15
Description: The event handler parses the packets from the protocol and determines whether if a proper event should be triggered as a response.
"""
# =============================================
#                   IMPORTS
# =============================================
import asyncio
import threading
from enum import Enum
from protocol import Protocol

# =============================================
#                   CONSTANTS
# =============================================
ADAFRUIT_BLE_MAC_ADDR = "C0:9E:48:AC:35:03"
MAX_PACKET_QUEUE_SIZE = 16

# =============================================
#                   CLASSES
# =============================================
class Events(Enum):
    # Music selection events
    MUSIC_SELECT_LEFT = 0,
    MUSIC_SELECT_RIGHT = 1,
    MUSIC_SELECT = 2,
    
    # Song skipping
    SONG_SKIP_PREV = 3,
    SONG_SKIP_NEXT = 4,
    
    # Playing/pausing sounds
    SONG_PLAY = 5,
    SONG_PAUSE = 6,
    

class EventHandler:
    def __init__(self, mac_address, max_packet_queue_size):
        """
        @name: __init__
        @param mac_address: The mac address of the Bluefruit you're using.
        @param max_packet_queue_size: The maximum number of packets that can be in the buffer at once.
        @return: None
        @brief: This is just an initialization function.
        """
        # Set up the protocol
        self._protocol = Protocol(mac_address, max_packet_queue_size)
        self._event_dict = dict() # Dictionary to store the events
        self._event_callback_dict = dict() # Dictionary to store callback functions
        
        # Set up thread to parse the packets
        self._thread = threading.Thread(target=self.__parse_packet, daemon=True)
        self._thread.start()
        
        # Go through the Enums and add them into the dictionary, ID is used to reference event, hence should be key, name is the value.
        for event in Events:
            self._event_dict[event.value] = event.name
        
    
    def __parse_packet(self):
        """
        @name: __parse_packet
        @param None
        @return: None
        @brief: This function parses the packets and raise events that corresponds to the packet ID
        """
        while True:
            # Grab the packet, if there is none, ignore
            packet = self._protocol.get_packet()
            if (packet is None):
                continue
            
            # Parse the packet
            # The payload is the 3rd item.
            # Within the payload, the first item is always the ID
            packet_payload = packet[2]
            packet_id = packet_payload[0]
            
            # Find the name of the event based on the ID
            event_name = self._event_dict.get(packet_id)
            if event_name is None: return
            
            # Find the corresponding callback function associated with the event name
            event_cb = self._event_callback_dict.get(event_name)
            if event_cb is None: return
            
            # Call the callback function, sending in the payload as the arguments
            event_cb(packet_payload)
    
    def on_event(self, event : Enum, event_callback):
        """
        @name: on_event
        @param: event -> The enum of the event, if the event doesn't exist, an error will be raised.
        @param: event_callback -> The callback function to call in response to the event being triggered.
        @return: None
        @brief: This function parses the packets and raise events that corresponds to the packet ID
        """
        
        # Case 1: Raise an error if the event doesn't exist in the Enum list.
        if not event in Events:
            raise Exception(f"The event {event.name} isn't a registered Event Enum, add it to Events enum before proceeding with this function.")
        
        # Case 2: The callback isn't actually a function
        if not callable(event_callback):
            raise Exception(f"An non-function was sent as the event_callback parameter.")
        
        # Case 3: If the event is already registered
        if event.name in self._event_callback_dict:
            raise Exception(f"The event {event.name} is already associated with another callback.")
        
        # Add the event along with the callback to the dictionary
        self._event_callback_dict[event.name] = event_callback