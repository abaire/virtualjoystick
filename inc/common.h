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

#ifndef __VJOYUM_COMMON_H__
#define __VJOYUM_COMMON_H__

//
// Custom control codes are defined here. They are to be used for sideband 
// communication with the hid minidriver. These control codes are sent to 
// the hid minidriver using Hid_SetFeature() API to a custom collection 
// defined especially to handle such requests.
//
#define  HIDMINI_CONTROL_CODE_SET_ATTRIBUTES              0x00
#define  HIDMINI_CONTROL_CODE_DUMMY1                      0x01
#define  HIDMINI_CONTROL_CODE_DUMMY2                      0x02

//
// This is the report id of the collection to which the control codes are sent
//
#define CONTROL_COLLECTION_REPORT_ID                      0x01
#define TEST_COLLECTION_REPORT_ID                         0x02

#define MAXIMUM_STRING_LENGTH           (126 * sizeof(WCHAR))
#define VHIDMINI_MANUFACTURER_STRING    L"BearBrains Inc."
#define VHIDMINI_PRODUCT_STRING         L"Virtual Joystick Multiplexer"
#define VHIDMINI_SERIAL_NUMBER_STRING   L"123123123"
#define VHIDMINI_DEVICE_STRING          L"Virtual Joystick Multiplexer device"
#define VHIDMINI_DEVICE_STRING_INDEX    5

#define HIDVJOY_CONTROL_CODE_SET_DATA    0x00
#define CONTROL_COLLECTION_REPORT_ID     0x01

#define HIDVJOY_VID 0xEBA0
#define HIDVJOY_PID 0x0001

#define REPORTID_JOYSTICK 0x05
#define REPORTID_VENDOR 0x40


#include <pshpack1.h>

typedef struct _MY_DEVICE_ATTRIBUTES
{
    USHORT VendorID;
    USHORT ProductID;
    USHORT VersionNumber;
} MY_DEVICE_ATTRIBUTES, *PMY_DEVICE_ATTRIBUTES;

typedef struct _HIDMINI_CONTROL_INFO
{
    //
    //report ID of the collection to which the control request is sent
    //
    UCHAR ReportId;

    //
    // One byte control code (user-defined) for communication with hid 
    // mini driver
    //
    UCHAR ControlCode;

    //
    // This union contains input data for the control request.
    //
    union
    {
        MY_DEVICE_ATTRIBUTES Attributes;

        struct
        {
            ULONG Dummy1;
            ULONG Dummy2;
        } Dummy;
    } u;
} HIDMINI_CONTROL_INFO, *PHIDMINI_CONTROL_INFO;

//
// input from device to system
//
typedef struct _HIDMINI_INPUT_REPORT
{
    UCHAR ReportId;

    UCHAR Data;
} HIDMINI_INPUT_REPORT, *PHIDMINI_INPUT_REPORT;

//
// output to device from system
//
typedef struct _HIDMINI_OUTPUT_REPORT
{
    UCHAR ReportId;

    UCHAR Data;

    USHORT Pad1;

    ULONG Pad2;
} HIDMINI_OUTPUT_REPORT, *PHIDMINI_OUTPUT_REPORT;





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
    UINT8            id;
    DEVICE_REPORT   report;
} DEVICE_PACKET, * PDEVICE_PACKET;


#include <poppack.h>

//
// SetFeature request requires that the feature report buffer size be exactly 
// same as the size of report described in the hid report descriptor (
// excluding the report ID). Since HIDMINI_CONTROL_INFO includes report ID,
// we subtract one from the size.
//
#define FEATURE_REPORT_SIZE_CB      ((USHORT)(sizeof(HIDMINI_CONTROL_INFO) - 1))
#define INPUT_REPORT_SIZE_CB        ((USHORT)(sizeof(HIDMINI_INPUT_REPORT) - 1))
#define OUTPUT_REPORT_SIZE_CB       ((USHORT)(sizeof(HIDMINI_OUTPUT_REPORT) - 1))

#endif // __VJOYUM_COMMON_H__
