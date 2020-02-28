/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    vhidmini.h

Abstract:

    This module contains the type definitions for the driver

Environment:

    Windows Driver Framework (WDF)

--*/

#pragma once

#include <windows.h>
#include <wdf.h>
#include <hidport.h>

#include "HIDReportDescriptor.h"

#define VJOYUM_MANUFACTURER_STRING    L"BearBrains Inc."
#define VJOYUM_PRODUCT_STRING         L"Virtual Joystick Multiplexer"
#define VJOYUM_SERIAL_NUMBER_STRING   L"123123123"
#define VJOYUM_DEVICE_STRING          L"Virtual Joystick Multiplexer device"
#define VJOYUM_DEVICE_STRING_INDEX    5

#define HIDVJOY_VERSION         0x0101

typedef struct _DEVICE_CONTEXT
{
    WDFDEVICE Device;
    WDFQUEUE DefaultQueue;
    WDFQUEUE ManualQueue;
    HID_DEVICE_ATTRIBUTES HidDeviceAttributes;
    BYTE DeviceData;
    HID_DESCRIPTOR HidDescriptor;
    PHID_REPORT_DESCRIPTOR ReportDescriptor;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext);

typedef struct _QUEUE_CONTEXT
{
    WDFQUEUE Queue;
    PDEVICE_CONTEXT DeviceContext;
} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, GetQueueContext);

NTSTATUS
QueueCreate(
    _In_ WDFDEVICE Device,
    _Out_ WDFQUEUE* Queue
);

DRIVER_INITIALIZE                   DriverEntry;

NTSTATUS
RequestCopyFromBuffer(
    _In_ WDFREQUEST Request,
    _In_ PVOID SourceBuffer,
    _When_(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    _In_ size_t NumBytesToCopyFrom
);
