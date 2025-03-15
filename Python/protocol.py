"""
protocol.py
Author: Derrick Lai
Date: 2025-03-11
Description: The messages received from the STM-Bluetooth from the UART is only a single byte (8 bits), this is due to the flow controls implemented
in the UART code in the STM32 to prevent massive amounts of data loss and garbage values. Hence, the PC (this device) must process each character (byte) to form
a packet following the Protocol agreed by both sides (the STM32 and PC).

The protocol is described below as follows:
Start Byte
Length
MessageID/EventID
Data/Payload
Checksum
End Byte

# Packet Structure (From ECE121's UART Protocol):
# +---------+--------+-----------+------+----------+------+
# |  HEAD   | LENGTH | PAYLOAD   | TAIL | CHECKSUM | END  |
# |  (1)    |  (1)   | (<128)    | (1)  |   (1)    | \r\n |
# +---------+--------+-----------+------+----------+------+
#
# - HEAD:      1 byte, Start of the packet
# - LENGTH:    1 byte, Length of the payload
# - PAYLOAD:   Variable length, 1 byte is guaranted to be the ID, but can hold up to 127 bytes of data. Upper limit is 128 bytes total (including length and data)
# - TAIL:      1 byte, End marker for payload
# - CHECKSUM:  1 byte, for validating the packet
# - END:       "\r\n", 2 bytes to indicate the end of the packet.

Sources:

- Switch Cases
https://www.freecodecamp.org/news/python-switch-statement-switch-case-example/

"""
# =============================================
#                   IMPORTS
# =============================================
import asyncio
import threading
import copy
from queue import Queue
from enum import Enum
from ble_comm import BluefruitComm

# =============================================
#                   CONSTANTS
# =============================================
# Define ASCII values for the Protocol components. Use ord() to convert a character to such values
HEAD = 204 # Equals 0xCC
TAIL = 185 # Equals 0xB9
CARRIAGE = 13 # Equals 0x0D or '\r'
NEWLINE = 10 # Equals 0x0A or '\n'

# =============================================
#                   CLASSES
# =============================================
"""
@class: PacketStates
@brief: These are the enums that will be used for the state machine to process and form packets. Here is a brief summary of the FSM.
AWAIT_HEAD - Checks for the head byte to begin packet building.
AWAIT_LENGTH - Wait for the next byte, assuming its the length.
AWAIT_PAYLOAD - Will keep taking data bytes in this state until it receives a tail.
AWAIT_CHKSUM - Waits for the checksum to compare with the computed checksum.
AWAIT_END_RC - Waits for the return carriage '\r' in the first part to signify the end of transmission.
AWAIT_END_NL - Waits for the newline '\n' as the final part to represent end of transmission and to form the full packet.
"""
class PacketStates(Enum):
    AWAIT_HEAD = 0,
    AWAIT_LENGTH = 1,
    AWAIT_ID = 2,
    AWAIT_PAYLOAD = 3,
    AWAIT_CHKSUM = 4,
    AWAIT_END_RC = 5,
    AWAIT_END_NL = 6

class Protocol:
    def __init__(self, mac_address, max_queue_size):
        """
        @name: __init__
        @param mac_address: The mac address of the Bluefruit that you want to connect to.
        @return: None
        @brief: Initializing to set up Bluefruit transmission/reception and a thread to process received characters for forming packets.
        """
        # Initialize the Bluefruit Client (for transmission and reception)
        self.bf_client = BluefruitComm(mac_address=mac_address)
        self.packet_queue = Queue(maxsize=max_queue_size)
        
        # Set up a thread to pull characters
        self._thread = threading.Thread(target=self.__receive_characters, daemon=True)
        self._thread.start()
        
        # State control for FSM (Initial starts starts by waiting for the head)
        self._temp_packet = list() # Packet will be stored in a list, payload will be another list inside.
        self._current_state = PacketStates.AWAIT_HEAD
        self._current_chk_sum = 0
        
    def get_packet(self):
        """
        @name: get_packet
        @param None
        @return: None
        @brief: Returns a fully-formed packet from the queue if there is one, otherwise, it will return None.
        """
        # Return none if empty.
        if self.packet_queue.empty():
            return None
        
        # Return the first item from the queue
        return self.packet_queue.get()
    
    def send_packet(self, data : list):
        """
        @name: get_packet
        @param data : The data payload (stored in a list, each value should be a single character (byte))
        @return: None
        @brief: Returns a fully-formed packet from the queue if there is one, otherwise, it will return None.
        """
        
        # Assemble the packet in list form.
        packet_to_send = list()
        
        # Set the head (we're sending the ASCII ints directly. On the plus side, GATT doesn't have to encode the strings)
        packet_to_send.append(HEAD)
        
        # Set the length (the length of the data array in the form of a string)
        packet_to_send.append(len(data))
        
        # Append the payload, also calculate the checksum
        checksum = 0
        for byte in data:
            packet_to_send.append(byte)
            checksum = self.__compute_iterative_checksum(byte, checksum) # The checksum takes in an integer
        
        # Set the tail
        packet_to_send.append(TAIL)
        
        # Set the checksum (convert to a equivalent character first)
        packet_to_send.append(checksum)
        
        # Set the two end bytes, the return carriage '\r' and newline 'n', convert to char first.
        packet_to_send.append(CARRIAGE)
        packet_to_send.append(NEWLINE)
        
        # Combine the values of the array into a string using .join()
        # packet_str = ''.join(packet_to_send)
        
        # print(f"Sending packet: {packet_str}")
        self.bf_client.send_message(packet_to_send)

    
    def __receive_characters(self):
        """
        @name: __receive_characters
        @param mac_address: None
        @return: None
        @brief: Checks the BluefruitComm's receive buffer for incoming ASCII integers, and if there is, 
        takes the character to update the FSM used to build the packet.
        """
        # print("Receiving Characters...")
        while (True):
            # Read from the buffer, if there is nothing, wait until the next iteration
            byte_int = self.bf_client.get_message()
            if byte_int is None:
                continue
            
            # Otherwise, update the FSM with the character
            # print("Received the character: ", byte_int)
            self.__update_fsm(byte_int)
    
    def __compute_iterative_checksum(self, char_byte : int, previous_chk_sum : int):
        """
        @name: __compute_iterative_checksum
        @param char_byte: The new character (represented in ASCII integer form) to compute the new checksum on.
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
    
    def __update_fsm(self, char_byte : str):
        """
        @name: __update_fsm
        @param char_byte -> A character (byte) that is part of the packet. The state machine will determine how its assembled. Note: Represented as an INT
        @return: None
        @brief: Starts the event loop necessary to run the thread that is in charge of receiving messages and building the
        packets.
        """
        # Check the state and perform the actions based on the state.
        # Convert the char into a byte format.
        match self._current_state:
            
            case PacketStates.AWAIT_HEAD:
                # If the incoming character matches the head, insert to packet and transition to next state.
                if (char_byte == HEAD):
                    self._temp_packet.append(char_byte)
                    self._current_state = PacketStates.AWAIT_LENGTH
                    
                # print("Head State")
            
            case PacketStates.AWAIT_LENGTH:
                # Take in the next character and include to packet, transition to next state.
                # Assume every character is the length (the checksum will validate this later)
                self._temp_packet.append(char_byte)
                self._current_state = PacketStates.AWAIT_ID
                
                # print("Length State")
            
            case PacketStates.AWAIT_ID:
                # Assume the next character is the ID, insert to payload data, then add to checksum for validation.
                payload_data = list()
                self._temp_packet.append(payload_data)
                payload_data.append(char_byte)
                
                # Append an integer (representing the checksum value) to the packet, it starts at 0
                check_sum = 0
                self._temp_packet.append(check_sum)
                
                # Checksum calculations, then replace old checksum with the new value.
                new_checksum = self.__compute_iterative_checksum(char_byte, check_sum)
                self._temp_packet[3] = new_checksum # The 3rd index (4th) item inserted is the checksum value.
                
                # Transition to next state
                self._current_state = PacketStates.AWAIT_PAYLOAD
                
                # print("ID State")
            
            case PacketStates.AWAIT_PAYLOAD:
                
                # print("Payload State")
                # Transition condition: If we received a tail value, transition states and exit.
                if (char_byte == TAIL):
                    self._temp_packet.append(char_byte)
                    self._current_state = PacketStates.AWAIT_CHKSUM
                    return
                
                # Keep assuming that the new character given is a payload value, update checksum
                payload_data : list = self._temp_packet[2] # 3rd item in packet is the payload list.
                payload_data.append(char_byte)
                new_checksum : int = self.__compute_iterative_checksum(char_byte, self._temp_packet[3])
                self._temp_packet[3] = new_checksum
            
            case PacketStates.AWAIT_CHKSUM:
                
                # print("Checksum State")
                
                # The next character is the value of the checksum, try to match the value with the calculated
                # checksum from the sent payload. If the values don't match, then the packet is invalid.
                # If this is the case, than clear the list and wait for a head.
                calc_checksum = self._temp_packet[3]
                
                if (calc_checksum != char_byte):
                    self._temp_packet.clear()
                    self._current_state = PacketStates.AWAIT_HEAD
                    return
                    
                # If the checksum matches, transition to the next state.
                self._current_state = PacketStates.AWAIT_END_RC
                
            
            case PacketStates.AWAIT_END_RC:
                
                # print("Return Carriage State")
                # If the new incoming character isn't a return carriage, then assume loss in transition
                # and try to restart the packet
                if (char_byte != CARRIAGE):
                    self._temp_packet.clear()
                    self._current_state = PacketStates.AWAIT_HEAD
                    return
                
                # Return carriage is given, transition to the next state
                self._current_state = PacketStates.AWAIT_END_NL
            
            case PacketStates.AWAIT_END_NL:
                
                # print("New Line State")
                # In every case, transition will be directly back to the head
                self._current_state = PacketStates.AWAIT_HEAD
                
                # Same principle applies as AWAIT_END_RC. The incoming character must be a new line, or else
                # a loss in transition is assume, packet will restart in this case
                if (char_byte != NEWLINE):
                    self._temp_packet.clear()
                    return
                
                # The packet is completed, send the packet to the queue for processing, then clear the array
                if (not self.packet_queue.full()):
                    self.packet_queue.put(copy.deepcopy(self._temp_packet))
                    self._temp_packet.clear()
            case _:
                raise("The protocol state machine has reached an undefined state!")