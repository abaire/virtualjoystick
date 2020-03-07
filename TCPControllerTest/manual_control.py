#!/usr/bin/env python3

import select
import socket
import struct
import sys
import time

# Must match the unpack in read_state and the actual struct contents
# of VirtualDeviceState in VJoyDriverInterface.cs.
_SIZEOF_DEVICE_STATE = (9 * 2) + 4 + 16 + 1 + 7


_AXIS_X = 0
_AXIS_Y = 1
_AXIS_THROTTLE = 2
_AXIS_RX = 3
_AXIS_RY = 4
_AXIS_RZ = 5
_AXIS_SLIDER = 6
_AXIS_DIAL = 7
_AXIS_RUDDER = 8

class DeviceState:
    def __init__(self):
        self.axes = [0] * 9

        self.pov_north = False
        self.pov_east = False
        self.pov_south = False
        self.pov_west = False

        self.buttons = [0] * 16
        self.modifier_keys = 0
        self.keycodes = [0] * 7

    def __repr__(self):
        return (
            f"<DeviceState: Axes: {self.axes} POV: {self.pov_north} "
            f"{self.pov_east} {self.pov_south} {self.pov_west} Buttons: "
            f"{self.buttons} Modifiers: {self.modifier_keys} Keys: "
            f"{self.keycodes} >")
        
    def __eq__(self, other):
        for idx, val in enumerate(self.axes):
            if other.axes[idx] != val:
                return False

        for idx, val in enumerate(self.buttons):
            if other.buttons[idx] != val:
                return False

        for idx, val in enumerate(self.keycodes):
            if other.keycodes[idx] != val:
                return False
            
        return (
            self.pov_north == other.pov_north and
            self.pov_east == other.pov_east and
            self.pov_south == other.pov_south and
            self.pov_west == other.pov_west and
            self.modifier_keys == other.modifier_keys)
    
        
    def read(self, connection):
        buf = b""
        while (len(buf) < _SIZEOF_DEVICE_STATE):
            chunk = connection.recv(_SIZEOF_DEVICE_STATE - len(buf))
            buf += chunk
        (self.axes[0],
         self.axes[1],
         self.axes[2],
         self.axes[3],
         self.axes[4],
         self.axes[5],
         self.axes[6],
         self.axes[7],
         self.axes[8],
         self.pov_north,
         self.pov_east,
         self.pov_south,
         self.pov_west,
         self.buttons[0],
         self.buttons[1],
         self.buttons[2],
         self.buttons[3],
         self.buttons[4],
         self.buttons[5],
         self.buttons[6],
         self.buttons[7],
         self.buttons[8],
         self.buttons[9],
         self.buttons[10],
         self.buttons[11],
         self.buttons[12],
         self.buttons[13],
         self.buttons[14],
         self.buttons[15],
         self.modifier_keys,
         self.keycodes[0],
         self.keycodes[1],
         self.keycodes[2],
         self.keycodes[3],
         self.keycodes[4],
         self.keycodes[5],
         self.keycodes[6]
        ) = struct.unpack("!9h4b16bb7b", buf)

        self.pov_north = self.pov_north != 0
        self.pov_east = self.pov_east != 0
        self.pov_south = self.pov_south != 0
        self.pov_west = self.pov_west != 0

    def set_button(self, button, is_on=True):
        offset = button >> 3
        bit = 1 << (button & 0x07)

        if is_on:
            self.buttons[offset] |= bit
        else:
            self.buttons[offset] &= ~bit


class Connection:
    
    _ACK = bytearray([0x01])
    _NET_TIMEOUT = 0.5

    _COMMAND_KEYCODE = 0x10
    _COMMAND_BUTTON = 0x11
    _COMMAND_AXIS = 0x12
    _COMMAND_POV = 0x13
    _COMMAND_ECHO = 0x55
    _COMMAND_GET_CURRENT_STATE = 0x56

    def __init__(self):
        self._connection = None

    def connect(self):
        address = ("127.0.0.1", 13057)
        self._connection = socket.create_connection(address, self._NET_TIMEOUT)
        self._connection.setblocking(True)
        #self._connection.settimeout(self._NET_TIMEOUT)

        self._handshake()

    def send(self, command):
        print(f"Send: {command}")
        self._connection.sendall(command)
        assert self._connection.recv(1) == self._ACK

    def _handshake(self):
        self._connection.send(b'bbvj' + bytearray([0x01]))

    def set_echo(connection, is_on=True):
        command = struct.pack("!Bb", self._COMMAND_ECHO, is_on)
        self.send(connection, command)

    def get_current_state(self):
        command = struct.pack("!B", _self.COMMAND_GET_CURRENT_STATE)
        self.send(command)
    
        state = DeviceState()
        state.read(self._connection)
        return state

    def set_key(self, keycode, is_on=True):
        command = struct.pack("!BLB", self._COMMAND_KEYCODE, keycode, is_on)
        self.send(command)

    def set_button(self, button, is_on=True):
        command = struct.pack("!BBB", self._COMMAND_BUTTON, button, is_on)
        self.send(command)

    def set_axis(self, axis, value):
        command = struct.pack("!BBh", self._COMMAND_AXIS, axis, value)
        self.send(command)

    def set_pov(self, state):
        command = struct.pack("!Bb", self._COMMAND_POV, state)
        self.send(command)

        
def main(args):
    conn = Connection()
    conn.connect()

    for _ in range(1000):
        conn.set_button(1)
        time.sleep(0.5)
        conn.set_button(1, False)
        time.sleep(0.5)
    
if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
