#pragma once

#include <windows.h>
#include <wdf.h>
#include <hidport.h>  // located in $(DDK_INC_PATH)/wdm

#include "vjoyum.h"

typedef struct _MANUAL_QUEUE_CONTEXT
{
    WDFQUEUE queue;
    DEVICE_CONTEXT* deviceContext;
    WDFTIMER timer;
} MANUAL_QUEUE_CONTEXT, *PMANUAL_QUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MANUAL_QUEUE_CONTEXT, GetManualQueueContext);


NTSTATUS RequestGetHidXferPacket_ToReadFromDevice(
    _In_ WDFREQUEST Request,
    _Out_ HID_XFER_PACKET* Packet
);

NTSTATUS RequestGetHidXferPacket_ToWriteToDevice(
    _In_ WDFREQUEST Request,
    _Out_ HID_XFER_PACKET* Packet
);


NTSTATUS ManualQueueCreate(
    _In_ WDFDEVICE device,
    _In_ EVT_WDF_TIMER timerFunc,
    _Out_ WDFQUEUE* queueRet
);

/*++

Routine Description:

    A helper function to copy specified bytes to the request's output memory

Arguments:

    Request - A handle to a framework request object.

    SourceBuffer - The buffer to copy data from.

    NumBytesToCopyFrom - The length, in bytes, of data to be copied.

Return Value:

    NTSTATUS

--*/
NTSTATUS
RequestCopyFromBuffer(
    _In_ WDFREQUEST request,
    _In_ PVOID sourceBuffer,
    _When_(numBytesToCopyFrom == 0, __drv_reportError(numBytesToCopyFrom cannot be zero))
    _In_ size_t numBytesToCopyFrom
);
