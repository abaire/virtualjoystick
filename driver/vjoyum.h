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
    WDFDEVICE device;
    WDFQUEUE defaultQueue;
    WDFQUEUE pendingReadQueue;
    UINT lastServedReportID;
    HID_DEVICE_ATTRIBUTES hidDeviceAttributes;
    HID_DESCRIPTOR hidDescriptor;
    PHID_REPORT_DESCRIPTOR reportDescriptor;
} DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext);

typedef struct _QUEUE_CONTEXT
{
    WDFQUEUE queue;
    DEVICE_CONTEXT* deviceContext;
} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, GetQueueContext);

DRIVER_INITIALIZE DriverEntry;
