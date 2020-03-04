/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    common.h

Environment:

    User mode

--*/

#pragma once

#include <windows.h>

#define HIDVJOY_VID 0xEBA0
#define HIDVJOY_PID 0x0001

#define REPORTID_JOYSTICK 0x05
#define REPORTID_KEYBOARD 0x06
#define REPORTID_VENDOR 0x40


#include <pshpack1.h>

typedef struct _JOYSTICK_SUBREPORT
{
    INT16 X;
    INT16 Y;

    INT16 Throttle;
    INT16 Rudder;

    INT16 rX;
    INT16 rY;
    INT16 rZ;

    INT16 Slider;
    INT16 Dial;

    INT8  POV;

    UCHAR Button[16];  // 128 button bits
} JOYSTICK_SUBREPORT;

typedef struct _KEYBOARD_SUBREPORT
{
    UINT8 modifierKeys;
    UINT8 keycodes[7];
} KEYBOARD_SUBREPORT;

typedef struct _VENDOR_DEVICE_PACKET
{
    UINT8 id;
    JOYSTICK_SUBREPORT joystick;
    KEYBOARD_SUBREPORT keyboard;
} VENDOR_DEVICE_PACKET;


#include <poppack.h>
