#!/usr/bin/env python3

import pytest
import select
import socket
import struct
import sys


_NET_TIMEOUT = 0.1
_CONN_CLOSED = b''
_ACK = bytearray([0x01])

_COMMAND_KEYCODE = 0x10
_COMMAND_BUTTON = 0x11
_COMMAND_AXIS = 0x12
_COMMAND_POV = 0x13


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
    
    
def test_invalid_handshake(raw_connection):
    raw_connection.sendall(b'saab' + bytearray([0x01]))
    ack = raw_connection.recv(1)
    assert ack == _CONN_CLOSED

    
def test_valid_handshake(raw_connection):
    raw_connection.sendall(b'bbvj' + bytearray([0x01]))
    ack = raw_connection.recv(1)
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
    
    
