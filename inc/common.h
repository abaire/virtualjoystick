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
#define REPORTID_VENDOR 0x40


#include <pshpack1.h>

typedef struct _DEVICE_REPORT
{
    INT16 X;
    INT16 Y;
    INT16 Z;

    INT16 rX;
    INT16 rY;
    INT16 rZ;

    INT16 Slider[2];  // 2 slider/dial axes
    INT8  POV[4];     // 4 POV's (valid values are -1,0,1,2,3,4,5,6,7)

    UCHAR Button[16]; // 128 button bits
} DEVICE_REPORT, * PDEVICE_REPORT;

typedef struct _Device_Packet
{
    UINT8 id;
    DEVICE_REPORT report;
} DEVICE_PACKET, * PDEVICE_PACKET;


#include <poppack.h>

