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

// Contents must match ReportDescriptor in HIDReportDescriptor. 
typedef struct _DEVICE_REPORT
{
    INT16 X;
    INT16 Y;

    INT16 Throttle;
    INT16 Rudder;

    INT16 rX;
    INT16 rY;
    INT16 rZ;

    INT16 Slider[4];
    INT16 Dial[4];

    INT16  POV[4];

    UCHAR Button[16];  // 128 button bits
} DEVICE_REPORT, * PDEVICE_REPORT;

typedef struct _Device_Packet
{
    UINT8 id;
    DEVICE_REPORT report;
} DEVICE_PACKET, * PDEVICE_PACKET;


#include <poppack.h>

