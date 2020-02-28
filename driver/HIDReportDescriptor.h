#pragma once

#include <windows.h>
#include <wdf.h>
#include <hidport.h>

typedef UCHAR HID_REPORT_DESCRIPTOR, * PHID_REPORT_DESCRIPTOR;

// See https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf for definitions

#define USAGE_PAGE_GENERIC_DESKTOP 0x01

// 4.2 Axis Usages
#define USAGE_X 0x30
#define USAGE_Y 0x31
#define USAGE_Z 0x32
#define USAGE_RX 0x33
#define USAGE_RY 0x34
#define USAGE_RZ 0x35

// 4.3 Miscellaneous Controls
#define USAGE_SLIDER 0x36
#define USAGE_DIAL 0x37
#define USAGE_WHEEL 0x38
#define USAGE_HAT_SWITCH 0x39
#define USAGE_START 0x3D
#define USAGE_SELECT 0x3E

#define USAGE_PAGE_SIMULATION_CONTROLS 0x02

#define USAGE_RUDDER 0xBA
#define USAGE_THROTTLE 0xBB

#define USAGE_PAGE_BUTTONS 0x09

extern HID_REPORT_DESCRIPTOR ReportDescriptor[];

// Response for IOCTL_HID_GET_DEVICE_DESCRIPTOR
extern HID_DESCRIPTOR HidDescriptor;