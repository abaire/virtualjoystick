#pragma once

#include <windows.h>
#include <wdf.h>
#include <hidport.h>  // located in $(DDK_INC_PATH)/wdm

#include "vjoyum.h"

typedef struct _MANUAL_QUEUE_CONTEXT
{
    WDFQUEUE Queue;
    PDEVICE_CONTEXT DeviceContext;
} MANUAL_QUEUE_CONTEXT, * PMANUAL_QUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MANUAL_QUEUE_CONTEXT, GetManualQueueContext);


NTSTATUS RequestGetHidXferPacket_ToReadFromDevice(
    _In_ WDFREQUEST Request,
    _Out_ HID_XFER_PACKET* Packet
);

NTSTATUS RequestGetHidXferPacket_ToWriteToDevice(
    _In_ WDFREQUEST Request,
    _Out_ HID_XFER_PACKET* Packet
);

/*++
Routine Description:

    This function creates a manual I/O queue to receive IOCTL_HID_READ_REPORT
    forwarded from the device's default queue handler.

    It also creates a periodic timer to check the queue and complete any pending
    request with data from the device. Here timer expiring is used to simulate
    a hardware event that new data is ready.

    The workflow is like this:

    - Hidclass.sys sends an ioctl to the miniport to read input report.

    - The request reaches the driver's default queue. As data may not be avaiable
      yet, the request is forwarded to a second manual queue temporarily.

    - Later when data is ready (as simulated by timer expiring), the driver
      checks for any pending request in the manual queue, and then completes it.

    - Hidclass gets notified for the read request completion and return data to
      the caller.

    On the other hand, for IOCTL_HID_WRITE_REPORT request, the driver simply
    sends the request to the hardware (as simulated by storing the data at
    DeviceContext->DeviceData) and completes the request immediately. There is
    no need to use another queue for write operation.

Arguments:

    Device - Handle to a framework device object.

    Queue - Output pointer to a framework I/O queue handle, on success.

Return Value:

    NTSTATUS

--*/
NTSTATUS
ManualQueueCreate(
    _In_ WDFDEVICE Device,
    _Out_ WDFQUEUE* Queue
);

/*++

Routine Description:

    Read "ReadFromRegistry" key value from device parameters in the registry.

Arguments:

    device - pointer to a device object.

Return Value:

    NT status code.

--*/
NTSTATUS CheckRegistryForDescriptor(WDFDEVICE Device);

/*++

Routine Description:

    Read HID report descriptor from registry

Arguments:

    device - pointer to a device object.

Return Value:

    NT status code.

--*/
NTSTATUS ReadDescriptorFromRegistry(WDFDEVICE Device);
