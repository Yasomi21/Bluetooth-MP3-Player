########################################
### Libraries
########################################
import asyncio
from bleak import BleakScanner, BleakClient

########################################
### Globals
########################################
ADAFRUIT_BLE_MAC_ADDR = "C0:9E:48:AC:35:03" # Found using scan_for_devices()

'''
# UUID RX of the Adafruit Bluetooth BLE
# https://learn.adafruit.com/introducing-the-adafruit-bluefruit-le-uart-friend/uart-service
# Base UUID: 6E400001-B5A3-F393-­E0A9-­E50E24DCCA9E
# Note: The '0001' part is very important, it is the base address, it is the default and does nothing,
# change it to '0x0002' for it to transmit from computer to Adafruit BLE?
# So TX UUID: 6E400002-B5A3-F393-­E0A9-­E50E24DCCA9E
# RX UUID: 6E400003-B5A3-F393-­E0A9-­E50E24DCCA9E
'''
ADAFRUIT_BLE_TX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
ADAFRUIT_BLE_RX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"


########################################
### Main
########################################
# Sources: https://pypi.org/project/bleak/


# scan_for_devices()
# This function uses the BleakScanner to attempt to find BLE (Bluetooth Low Energy) devices.
# Note: This is an asynchronous function, it is a co-routine, it allows other functions to run while it performs its stuff in the background
async def scan_for_devices():
    print("Scanning for BLE devices.")
    devices = await BleakScanner.discover()
    for d in devices:
        print(d)

# send_message(msg : str)
# @Params: msg -> A string to send as the message.
# @Params: count -> How many times to send the message.
# @Params: delay -> How long to delay after each message
# Note: This is an asynchronous function, a co-routine.
async def send_message(msg : str, count : int, delay : float):
    # Attempt to write a message to the Adafruit bluetooth
    # First, establish the address as a client.
    async with BleakClient(ADAFRUIT_BLE_MAC_ADDR) as client:
        print("Connection made with Bluefruit...")
        
    #     # Transmit a message to the Bluefruit and wait for a respopnse
    #     await client.write_gatt_char(ADAFRUIT_BLE_TX_UUID, msg.encode(), response=True)
    #     print(f"Sent Message: {msg}")
        
    #     # Read the UART buffer to see if anything if message was received.
    #     try:
    #         response = await client.read_gatt_char(ADAFRUIT_BLE_TX_UUID)
    #     except Exception as e:
    #         print("Could not verify reception (this may be expected).", e)
        
        
        # Send the message in succession 'n' times, where n is the count
        for i in range(count):
            # Send the message, use encode() to convert strings to byte-format, don't wait for response...
            await client.write_gatt_char(ADAFRUIT_BLE_TX_UUID, msg.encode(), response=False)
            await asyncio.sleep(delay)
            print(f"Sent Message: {msg}")
        
        print("All messages printed.")


### Data Reception from the PC ###
# It seems to work for the counter (although data is a bit messy, we might need to transmit bit by bit with a communication protocol)
def notification_handler(sender, data):
    print(f"Sender: {sender} -> Message: {data.decode()}")
    
async def receive_message():
    # Establish connection with the address as a client
    async with BleakClient(ADAFRUIT_BLE_MAC_ADDR) as client:
        # Notify that connection was established
        print("Connection made with Bluefruit...")
        
        # Enable notification to receive incoming messsages using the receive ID and callback to handle messages.
        await client.start_notify(ADAFRUIT_BLE_RX_UUID, notification_handler)
        
        print("Listening to messages...")
        try:
            while (True):
                await asyncio.sleep(1)
        except KeyboardInterrupt:
            print("Stopping notifications...")
            client.stop_notify(ADAFRUIT_BLE_RX_UUID)
        
        

# We were able to get reception to work, we were able to get messages to be transmitted from the STM32's UART to the Bluefruit, then to the PC.
asyncio.run(receive_message())

# Send messsages once every second 1000 times.
#asyncio.run(send_message("Hello World!", 1000, 1))

#asyncio.run(send_message("Hello World!", 5, 0.1))
#asyncio.run(scan_for_devices())

# Alternative Source: https://github.com/Jakeler/ble-serial/blob/main/README.md
# GATT? https://learn.adafruit.com/introducing-the-adafruit-bluefruit-le-uart-friend/ble-gatt