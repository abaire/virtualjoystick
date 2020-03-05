/*++

Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    util.c

Abstract:

    This module contains the implementation of the driver

Environment:

    Windows Driver Framework (WDF)

--*/

#include "util.h"

//
// HID minidriver IOCTL uses HID_XFER_PACKET which contains an embedded pointer.
//
//   typedef struct _HID_XFER_PACKET {
//     PUCHAR reportBuffer;
//     ULONG  reportBufferLen;
//     UCHAR  reportId;
//   } HID_XFER_PACKET, *PHID_XFER_PACKET;
//
// UMDF cannot handle embedded pointers when marshalling buffers between processes.
// Therefore a special driver mshidumdf.sys is introduced to convert such IRPs to
// new IRPs (with new IOCTL name like IOCTL_UMDF_HID_Xxxx) where:
//
//   reportBuffer - passed as one buffer inside the IRP
//   reportId     - passed as a second buffer inside the IRP
//
// The new IRP is then passed to UMDF host and driver for further processing.
//

NTSTATUS RequestGetHidXferPacket_ToReadFromDevice(
    _In_ WDFREQUEST Request,
    _Out_ HID_XFER_PACKET* Packet
)
{
    //
    // Driver need to write to the output buffer (so that App can read from it)
    //
    //   Report Buffer: Output Buffer
    //   Report Id    : Input Buffer
    //

    NTSTATUS status;
    WDFMEMORY inputMemory;
    WDFMEMORY outputMemory;
    size_t inputBufferLength;
    size_t outputBufferLength;
    PVOID inputBuffer;
    PVOID outputBuffer;

    //
    // Get report Id from input buffer
    //
    status = WdfRequestRetrieveInputMemory(Request, &inputMemory);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfRequestRetrieveInputMemory failed 0x%x\n",status));
        return status;
    }
    inputBuffer = WdfMemoryGetBuffer(inputMemory, &inputBufferLength);

    if (inputBufferLength < sizeof(UCHAR))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        KdPrint(("WdfRequestRetrieveInputMemory: invalid input buffer. size %d, expect %d\n",
            (int)inputBufferLength, (int)sizeof(UCHAR)));
        return status;
    }

    Packet->reportId = *(PUCHAR)inputBuffer;

    //
    // Get report buffer from output buffer
    //
    status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfRequestRetrieveOutputMemory failed 0x%x\n",status));
        return status;
    }

    outputBuffer = WdfMemoryGetBuffer(outputMemory, &outputBufferLength);

    Packet->reportBuffer = (PUCHAR)outputBuffer;
    Packet->reportBufferLen = (ULONG)outputBufferLength;

    return status;
}

NTSTATUS RequestGetHidXferPacket_ToWriteToDevice(
    _In_ WDFREQUEST Request,
    _Out_ HID_XFER_PACKET* Packet
)
{
    //
    // Driver need to read from the input buffer (which was written by App)
    //
    //   Report Buffer: Input Buffer
    //   Report Id    : Output Buffer Length
    //
    // Note that the report id is not stored inside the output buffer, as the
    // driver has no read-access right to the output buffer, and trying to read
    // from the buffer will cause an access violation error.
    //
    // The workaround is to store the report id in the OutputBufferLength field,
    // to which the driver does have read-access right.
    //

    NTSTATUS status;
    WDFMEMORY inputMemory;
    WDFMEMORY outputMemory;
    size_t inputBufferLength;
    size_t outputBufferLength;
    PVOID inputBuffer;

    //
    // Get report Id from output buffer length
    //
    status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfRequestRetrieveOutputMemory failed 0x%x\n",status));
        return status;
    }
    WdfMemoryGetBuffer(outputMemory, &outputBufferLength);
    Packet->reportId = (UCHAR)outputBufferLength;

    //
    // Get report buffer from input buffer
    //
    status = WdfRequestRetrieveInputMemory(Request, &inputMemory);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfRequestRetrieveInputMemory failed 0x%x\n",status));
        return status;
    }
    inputBuffer = WdfMemoryGetBuffer(inputMemory, &inputBufferLength);

    Packet->reportBuffer = (PUCHAR)inputBuffer;
    Packet->reportBufferLen = (ULONG)inputBufferLength;

    return status;
}

NTSTATUS ManualQueueCreate(
    _In_ WDFDEVICE device,
    _In_ EVT_WDF_TIMER timerFunc,
    _Out_ WDFQUEUE* queueRet
)
{
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES queueAttributes;
    WDFQUEUE queue;
    PMANUAL_QUEUE_CONTEXT queueContext;
    WDF_TIMER_CONFIG timerConfig;
    WDF_OBJECT_ATTRIBUTES timerAttributes;

    WDF_IO_QUEUE_CONFIG_INIT(
        &queueConfig,
        WdfIoQueueDispatchManual);
    queueConfig.PowerManaged = WdfFalse;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &queueAttributes,
        MANUAL_QUEUE_CONTEXT);

    status = WdfIoQueueCreate(
        device,
        &queueConfig,
        &queueAttributes,
        &queue);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    queueContext = GetManualQueueContext(queue);
    queueContext->queue = queue;
    queueContext->deviceContext = GetDeviceContext(device);

    WDF_TIMER_CONFIG_INIT(&timerConfig, timerFunc);

    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    timerAttributes.ParentObject = queue;
    status = WdfTimerCreate(&timerConfig,
                            &timerAttributes,
                            &queueContext->timer);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfTimerCreate failed 0x%x\n", status));
        return status;
    }

    *queueRet = queue;

    return status;
}

NTSTATUS
RequestCopyFromBuffer(
    _In_ WDFREQUEST request,
    _In_ PVOID sourceBuffer,
    _When_(numBytesToCopyFrom == 0, __drv_reportError(numBytesToCopyFrom cannot be zero))
    _In_ size_t numBytesToCopyFrom
)
{
    NTSTATUS status;
    WDFMEMORY memory;
    size_t outputBufferLength;

    status = WdfRequestRetrieveOutputMemory(request, &memory);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfRequestRetrieveOutputMemory failed 0x%x\n", status));
        return status;
    }

    WdfMemoryGetBuffer(memory, &outputBufferLength);
    if (outputBufferLength < numBytesToCopyFrom)
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        KdPrint(("RequestCopyFromBuffer: buffer too small. Size %d, expect %d\n",
            (int)outputBufferLength, (int)numBytesToCopyFrom));
        return status;
    }

    status = WdfMemoryCopyFromBuffer(memory,
                                     0,
                                     sourceBuffer,
                                     numBytesToCopyFrom);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfMemoryCopyFromBuffer failed 0x%x\n", status));
        return status;
    }

    WdfRequestSetInformation(request, numBytesToCopyFrom);
    return status;
}
