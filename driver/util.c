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

NTSTATUS
RequestGetHidXferPacket_ToReadFromDevice(
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

NTSTATUS
RequestGetHidXferPacket_ToWriteToDevice(
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
    _In_ WDFDEVICE Device,
    _Out_ WDFQUEUE* Queue
)
{
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES queueAttributes;
    WDFQUEUE queue;
    PMANUAL_QUEUE_CONTEXT queueContext;

    WDF_IO_QUEUE_CONFIG_INIT(
        &queueConfig,
        WdfIoQueueDispatchManual);
    queueConfig.PowerManaged = WdfFalse;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &queueAttributes,
        MANUAL_QUEUE_CONTEXT);

    status = WdfIoQueueCreate(
        Device,
        &queueConfig,
        &queueAttributes,
        &queue);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    queueContext = GetManualQueueContext(queue);
    queueContext->Queue = queue;
    queueContext->DeviceContext = GetDeviceContext(Device);

    *Queue = queue;

    return status;
}

NTSTATUS CheckRegistryForDescriptor(WDFDEVICE Device)
{
    WDFKEY hKey = NULL;
    NTSTATUS status;
    UNICODE_STRING valueName;
    ULONG value;

    status = WdfDeviceOpenRegistryKey(Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      KEY_READ,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &hKey);
    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&valueName, L"ReadFromRegistry");

        status = WdfRegistryQueryULong(hKey,
                                       &valueName,
                                       &value);

        if (NT_SUCCESS(status))
        {
            if (value == 0)
            {
                status = STATUS_UNSUCCESSFUL;
            }
        }

        WdfRegistryClose(hKey);
    }

    return status;
}

NTSTATUS ReadDescriptorFromRegistry(WDFDEVICE Device)
{
    WDFKEY hKey = NULL;
    NTSTATUS status;
    UNICODE_STRING valueName;
    WDFMEMORY memory;
    size_t bufferSize;
    PVOID reportDescriptor;
    PDEVICE_CONTEXT deviceContext;
    WDF_OBJECT_ATTRIBUTES attributes;

    deviceContext = GetDeviceContext(Device);

    status = WdfDeviceOpenRegistryKey(Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      KEY_READ,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &hKey);

    if (!NT_SUCCESS(status))
    {
        return status;
    }
    RtlInitUnicodeString(&valueName, L"MyReportDescriptor");

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Device;

    status = WdfRegistryQueryMemory(hKey,
                                    &valueName,
                                    NonPagedPool,
                                    &attributes,
                                    &memory,
                                    NULL);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    reportDescriptor = WdfMemoryGetBuffer(memory, &bufferSize);

    KdPrint(("No. of report descriptor bytes copied: %d\n", (INT)bufferSize));

    //
    // Store the registry report descriptor in the device extension
    //
    deviceContext->ReadReportDescFromRegistry = TRUE;
    deviceContext->ReportDescriptor = reportDescriptor;
    deviceContext->HidDescriptor.DescriptorList[0].wReportLength = (USHORT)bufferSize;

    WdfRegistryClose(hKey);

    return status;
}
