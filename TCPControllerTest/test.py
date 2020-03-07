#!/usr/bin/env python3

import pytest
import select
import socket
import struct
import sys


_NET_TIMEOUT = 0.5
_CONN_CLOSED = b''
_ACK = bytearray([0x01])

_COMMAND_KEYCODE = 0x10
_COMMAND_BUTTON = 0x11
_COMMAND_AXIS = 0x12
_COMMAND_POV = 0x13
_COMMAND_ECHO = 0x55

# Must match the unpack in read_state and the actual struct contents
# of VirtualDeviceState in VJoyDriverInterface.cs.
_SIZEOF_DEVICE_STATE = (9 * 2) + 4 + 16 + 1 + 7

class DeviceState:
    def __init__(self):
        self.x = 0
        self.y = 0
        self.throttle = 0
        self.rudder = 0
        self.rx = 0
        self.ry = 0
        self.rz = 0
        self.slider = 0
        self.dial = 0

        self.pov_north = False
        self.pov_east = False
        self.pov_south = False
        self.pov_west = False

        self.buttons = [0] * 16
        self.modifier_keys = 0
        self.keycodes = [0] * 7

    def __repr__(self):
        return (
            f"<DeviceState: Axes: {self.x} {self.y} {self.throttle} "
            f"{self.rudder} {self.rx} {self.ry} {self.rz} {self.slider} "
            f"{self.dial} POV: {self.pov_north} {self.pov_east} "
            f"{self.pov_south} {self.pov_west} Buttons: {self.buttons} "
            f"Modifiers: {self.modifier_keys} Keys: {self.keycodes} >")
        
    def __eq__(self, other):
        for idx, val in enumerate(self.buttons):
            if other.buttons[idx] != val:
                return False

        for idx, val in enumerate(self.keycodes):
            if other.keycodes[idx] != val:
                return False
            
        return (
            self.x == other.x and
            self.y == other.y and
            self.throttle == other.throttle and
            self.rudder == other.rudder and
            self.rx == other.rx and
            self.ry == other.ry and
            self.rz == other.rz and
            self.slider == other.slider and
            self.dial == other.dial and
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
        (self.x,
         self.y,
         self.throttle,
         self.rudder,
         self.rx,
         self.ry,
         self.rz,
         self.slider,
         self.dial,
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

    
def _connect():
    address = ("127.0.0.1", 13057)
    conn = socket.create_connection(address, _NET_TIMEOUT)
    conn.settimeout(_NET_TIMEOUT)
    return conn

                 
@pytest.fixture
def raw_connection():
    conn = _connect()
    yield conn
    conn.shutdown(2)
    conn.close()

                 
@pytest.fixture
def connection():
    conn = _connect()
    conn.sendall(b'bbvj' + bytearray([0x01]))
    ack = conn.recv(1)
    assert ack == _ACK
    yield conn
    conn.shutdown(2)
    conn.close()

                 
@pytest.fixture
def echo_connection(connection):
    command = struct.pack("!Bb", _COMMAND_ECHO, 1)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _ACK
    yield connection


def KeyCommand(keycode, is_on=True):
    return struct.pack("!BLB", _COMMAND_KEYCODE, keycode, is_on)


def ButtonCommand(button, is_on=True):
    return struct.pack("!BBB", _COMMAND_BUTTON, button, is_on)

    
def AxisCommand(axis, value):
    return struct.pack("!BhB", _COMMAND_AXIS, value, axis)


def POVCommand(state):
    return struct.pack("!Bb", _COMMAND_POV, state)
    
    
def test_invalid_handshake(raw_connection):
    raw_connection.sendall(b'saab' + bytearray([0x01]))
    ack = raw_connection.recv(1)
    assert ack == _CONN_CLOSED

    
def test_valid_handshake(raw_connection):
    raw_connection.sendall(b'bbvj' + bytearray([0x01]))
    ack = raw_connection.recv(1)
    assert ack == _ACK

    
def test_echo_on(connection):
    command = struct.pack("!Bb", _COMMAND_ECHO, 1)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _ACK
        

def test_invalid_command(connection):
    connection.sendall(bytearray([0x00]))
    assert connection.recv(1) == _CONN_CLOSED

    
def test_keyboard_command(connection):
    command = struct.pack("!BLB", _COMMAND_KEYCODE, ord('a'), True)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _ACK

    
def test_button_command(connection):
    command = struct.pack("!BBB", _COMMAND_BUTTON, 4, True)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _ACK

    
def test_button_command_out_of_range(connection):
    command = struct.pack("!BBB", _COMMAND_BUTTON, 128, True)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _CONN_CLOSED

    
def test_axis_command(connection):
    command = struct.pack("!BhB", _COMMAND_AXIS, 32767, 3)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _ACK

    
def test_axis_command_negative(connection):
    command = struct.pack("!BhB", _COMMAND_AXIS, -32767, 3)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _ACK

    
def test_pov_command(connection):
    command = struct.pack("!BB", _COMMAND_POV, 3)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _ACK
    

def test_pov_command_out_of_range(connection):
    command = struct.pack("!BB", _COMMAND_POV, 9)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _CONN_CLOSED
    
    
def test_pov_command_out_of_range_negative(connection):
    command = struct.pack("!Bb", _COMMAND_POV, -1)
    connection.sendall(command)
    ack = connection.recv(1)
    assert ack == _CONN_CLOSED
    

def test_echo_button_change(echo_connection):
    command = ButtonCommand(0)
    echo_connection.sendall(command)
    assert echo_connection.recv(1) == _ACK

    state = DeviceState()
    state.read(echo_connection)

    expected = DeviceState()
    expected.set_button(0)

    assert state == expected


def test_echo_axis_change(echo_connection):
    command = AxisCommand(0, 1500)
    echo_connection.sendall(command)
    assert echo_connection.recv(1) == _ACK

    state = DeviceState()
    state.read(echo_connection)

    expected = DeviceState()
    expected.x = 1500

    assert state == expected

