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
        # Send the message in succession 'n' times, where n is the count
        for i in range(count):
            # Send the message, use encode() to convert strings to byte-format, don't wait for response...
            await client.write_gatt_char(ADAFRUIT_BLE_TX_UUID, msg.encode(), response=False)
            await asyncio.sleep(delay)
            print(f"Sent Message: {msg}")
        
        print("All messages printed.")

        
asyncio.run(send_message("Hello World!", 5, 0.1))

# Alternative Source: https://github.com/Jakeler/ble-serial/blob/main/README.md
# GATT? https://learn.adafruit.com/introducing-the-adafruit-bluefruit-le-uart-friend/ble-gatt