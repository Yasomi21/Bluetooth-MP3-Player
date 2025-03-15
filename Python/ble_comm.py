"""
ble_comm.py
Author: Derrick Lai
Date: 2025-03-11
Description: This script is the lowest level in the interface. It is in charge of receiving and transmitting messages to the
Adafruit Bluefruit LE UART Friend.

Sources:
https://bleak.readthedocs.io/en/latest/api/client.html
https://docs.python.org/3/library/asyncio.html
https://docs.python.org/3/library/asyncio-eventloop.html
https://docs.python.org/3/library/asyncio-queue.html#examples
https://docs.python.org/3/library/queue.html#module-queue

Async Event Loops:
https://stackoverflow.com/questions/51775413/python-asyncio-run-coroutine-threadsafe-never-running-coroutine

Byte Array to Integer List:
https://stackoverflow.com/questions/58795086/python-convert-a-byte-array-back-to-list-of-int

"""
# =============================================
#                   IMPORTS
# =============================================
import asyncio
import threading
from queue import Queue
from bleak import BleakScanner, BleakClient

# =============================================
#                   CONSTANTS
# =============================================
'''
# UUID RX of the Adafruit Bluetooth BLE
# https://learn.adafruit.com/introducing-the-adafruit-bluefruit-le-uart-friend/uart-service
# Base UUID: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
# Note: The '0001' part is very important, it is the base address, it is the default and does nothing,
# change it to '0x0002' for it to transmit from computer to Adafruit BLE?
# So TX UUID: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
# RX UUID: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
#
# As for the MAC Address, this is found from BleakScanner.discover(), which provides a list of devices with the MAC address and name.
'''
ADAFRUIT_BLE_MAC_ADDR = "C0:9E:48:AC:35:03"
ADAFRUIT_BLE_TX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
ADAFRUIT_BLE_RX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

MAX_BUFFER_SIZE = 16 # Maximum size of the circular buffer for incoming messages
# =============================================
#                   CLASSES
# =============================================
class BluefruitComm:
    def __init__(self, mac_address):
        """
        @name: __init__
        @param mac_address: The mac address of the Bluefruit you're using.
        @param event_loop: The asyncio event loop for running co-routines
        @param queue: The queue used to store packets for transmission
        @return: None
        @brief: This is just an initialization function.
        """
        # Take MAC address and set up a client
        self.mac_address = mac_address
        self._client = BleakClient(mac_address)
        self._client_connected = False
        
        # Queue to store received messages/characters (we don't have to worry about memory, so no need for circular buffer)
        self.receive_buffer = Queue(maxsize=MAX_BUFFER_SIZE)
        
        # Set up the event loop and queue (for outgoing messages) to prepare to be used by a thread (multithreading is used to contain this thread)
        self.event_loop = asyncio.new_event_loop()
        self.packet_queue = asyncio.Queue()
        
        # Create a thread to run the event loop and start the thread
        self._thread = threading.Thread(target=self.start_event_loop, daemon=True)
        self._thread.start()
        
        # Blocking Code: Wait until the event loop is running, it must run before co-routines can be submitted
        while (not self.event_loop.is_running()):
            pass
        
        # Start the co-routines for the asynchronous functions (Note: This must be done after thread has been created, or else it may run in the wrong thread)
        asyncio.run_coroutine_threadsafe(self.connect(), self.event_loop)
        asyncio.run_coroutine_threadsafe(self.process_tx_queue(), self.event_loop)
            
    def start_event_loop(self):
        """
        @name: start_event_loop
        @param None
        @return: None
        @brief: Starts up a permanent event loop.
        """
        # Start the event loop and leave it to run forever
        asyncio.set_event_loop(self.event_loop)
        self.event_loop.run_forever()
    
    async def connect(self):
        """
        @name: connect
        @param None
        @return: None
        @brief: Establishes a connection with the Bluefruit
        """
        print("Attempting connection to Bluefruit...")
        # Connect with the Bluefruit
        try:
            await self._client.connect()
        except Exception as e:
            print(f"Error occured during connection attempt: {e}")
            
        if (self._client.is_connected):
            # Set connection flag to True
            self._client_connected = True
            print(f"Connected to Bluefruit with address {self.mac_address}")
            # Establish a connection with incoming message events (you're sending it to the Bluefruit's Receive).
            await self._client.start_notify(ADAFRUIT_BLE_RX_UUID, self.on_tx_notify)
        else:
            print(f"Failed to make a connection with the Bluefruit")
    
    def disconnect(self):
        """
        @name: disconnect
        @param None
        @return: None
        @brief: Disconnect the Bluefruit from the device.
        """
        # Disconnect the Bluetooth Bleak client (since it is an async function, it must be done inside an event loop)
        asyncio.run_coroutine_threadsafe(self._client.disconnect(), self.event_loop)
        
        # Kill the co-routines
        self.event_loop.close()
    
    async def on_tx_notify(self, sender, message):
        """
        @name: on_tx_notify
        @param None
        @return: None
        @brief: When a message has been received by the PC, place it into the buffer
        """
         
        # The message sent is a byte array. If the character is not ASCII, the byte array will represent it as a hex '\0x84' for example.
        # However, if it is an ASCII character, such as 0x25, it will automaticalily convert to ASCII, to '%' for example.
        # Convert the byte values into raw integer values, go through each value and send it into the buffer
        for byte in list(message):
            
            # Drop characters if buffer is full.
            if self.receive_buffer.full():
                break
            
            # Send to the buffer
            self.receive_buffer.put(byte)
    
    async def process_tx_queue(self):
        """
        @name: process_incoming
        @param None
        @return: None
        @brief: Processes messages that are in the packet queue, takes each message and transmits it
        """
        while True:
            # If the client isn't connected, then no messages should be processed, wait until the client is connected.
            if (not self._client.is_connected):
                await asyncio.sleep(1)
                continue
            
            # Wait until the packet queue has a messsage
            packet_msg = await self.packet_queue.get()
            
            # Transmit the packet (defaults to UTF-8, GATT takes in bytes)
            await self._client.write_gatt_char(ADAFRUIT_BLE_TX_UUID, packet_msg.encode())
            # print("Message transmitted...")
        
    def get_message(self) -> str:
        """
        @name: get_message
        @param None
        @return: None
        @brief: Reads from the receive buffer to obtain data, should be one byte.
        Note: This byte of data is in integer form. Follow ASCII convention to figure out the character.
        """
        # If the receive queue is empty, return None
        if self.receive_buffer.empty():
            return None
        
        # Return the item
        return self.receive_buffer.get()
    
    def send_message(self, protocol_packet) -> None:
        """
        @name: send_message
        @param protocol_packet: The fully assembled protocol packet that is ready to be transmitted.
        @return: None
        @brief: Sends a message by placing it onto the queue.
        """
        # If the Bluefruit isn't connected yet, don't send any messages (since this is a non async function, I used a flag instead.)
        if (not self._client_connected):
            return
        
        # Why not circular buffer, because the async loop doesn't let you do while True readBuffer(), but await queue.get() is allowed.
        # Create a coroutine safe thread to place the item onto the packet queue (it should stop once item has successfully been placed)
        asyncio.run_coroutine_threadsafe(self.packet_queue.put(protocol_packet), self.event_loop)
    
    
    