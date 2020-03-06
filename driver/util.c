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

NTSTATUS RequestGetHidXferPacket_ToReadFromDevice(
    _In_ WDFREQUEST Request,
    _Out_ HID_XFER_PACKET* Packet
)
{
    NTSTATUS status;
    WDFMEMORY inputMemory;
    WDFMEMORY outputMemory;
    size_t inputBufferLength;
    size_t outputBufferLength;
    PVOID inputBuffer;
    PVOID outputBuffer;

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
    NTSTATUS status;
    WDFMEMORY inputMemory;
    WDFMEMORY outputMemory;
    size_t inputBufferLength;
    size_t outputBufferLength;
    PVOID inputBuffer;

    status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfRequestRetrieveOutputMemory failed 0x%x\n",status));
        return status;
    }
    WdfMemoryGetBuffer(outputMemory, &outputBufferLength);
    Packet->reportId = (UCHAR)outputBufferLength;

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
