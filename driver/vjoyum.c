#include "vjoyum.h"
#include "common.h"
#include "util.h"
#include "HIDReportDescriptor.h"

#define T_ERROR         DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL
#define T_WARNING       DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL
#define T_TRACE         DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL
#define T_INFO          DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL

// Maximum number of milliseconds to wait for information from the feeder
// application before resetting driver state. This prevents a disconnect
// from spamming infinite keyboard events.
#define FEEDER_DISCONNECT_TIMEOUT 1000

#define DUMP_VENDOR_DEVICE_REPORT( _v_ ) KdPrintEx(( \
    T_WARNING, \
    "\tX: 0x%X\n" \
    "\tY: 0x%X\n" \
    "\tThrottle: 0x%X\n" \
    "\tRudder: 0x%X\n" \
    "\trX: 0x%X\n" \
    "\trY: 0x%X\n" \
    "\trZ: 0x%X\n" \
    "\tSlider: 0x%X\n" \
    "\tDial: 0x%X\n" \
    "\tPOV: 0x%X]n" \
    "\tButton[0]: 0x%X\n" \
    "\tButton[1]: 0x%X\n" \
    "\tButton[2]: 0x%X\n" \
    "\tButton[3]: 0x%X\n" \
    "\tButton[4]: 0x%X\n" \
    "\tButton[5]: 0x%X\n" \
    "\tButton[6]: 0x%X\n" \
    "\tButton[7]: 0x%X\n" \
    "\tButton[8]: 0x%X\n" \
    "\tButton[9]: 0x%X\n" \
    "\tButton[10]: 0x%X\n" \
    "\tButton[11]: 0x%X\n" \
    "\tButton[12]: 0x%X\n" \
    "\tButton[13]: 0x%X\n" \
    "\tButton[14]: 0x%X\n" \
    "\tButton[15]: 0x%X\n" \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.X \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Y \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Throttle \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Rudder \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.rX \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.rY \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.rZ \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Slider \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Dial \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.POV \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[0] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[1] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[2] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[3] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[4] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[4] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[5] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[6] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[7] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[8] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[10] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[11] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[12] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[13] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[14] \
    , ((VENDOR_DEVICE_PACKET*)(_v_))->joystick.Button[15] ))

#include <pshpack1.h>

typedef struct _JOYSTICK_REPORT
{
    UINT8 id;
    JOYSTICK_SUBREPORT data;
} JOYSTICK_REPORT;

typedef struct _KEYBOARD_REPORT
{
    UINT8 id;
    KEYBOARD_SUBREPORT data;
} KEYBOARD_REPORT;

#include <poppack.h>

static EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
static EVT_WDF_TIMER EvtTimer;

static NTSTATUS QueueCreate(
    _In_ WDFDEVICE Device,
    _Out_ WDFQUEUE* Queue
);

static NTSTATUS ReadReport(
    _In_ PQUEUE_CONTEXT QueueContext,
    _In_ WDFREQUEST Request,
    _Always_(_Out_)
    BOOLEAN* CompleteRequest
);

static NTSTATUS DequeuePendingReadRequest(
    _In_ WDFQUEUE queue,
    _In_ WDFREQUEST* reportRequest);

static NTSTATUS HandlePendingJoystickReadReportRequest(
    _In_ WDFQUEUE queue,
    _In_ JOYSTICK_SUBREPORT* pPacket,
    _Out_ ULONG* bytesWritten
);

static NTSTATUS HandlePendingKeyboardReadReportRequest(
    _In_ WDFQUEUE queue,
    _In_ KEYBOARD_SUBREPORT* keyboardState,
    _Out_ ULONG* bytesWritten
);

static NTSTATUS WriteReport(
    _In_ PQUEUE_CONTEXT queueContext,
    _In_ WDFREQUEST request
);

static NTSTATUS GetString(
    _In_ WDFREQUEST Request
);

static NTSTATUS GetIndexedString(
    _In_ WDFREQUEST Request
);

static NTSTATUS GetStringId(
    _In_ WDFREQUEST Request,
    _Out_ ULONG* StringId,
    _Out_ ULONG* LanguageId
);

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS, or another status value for which NT_SUCCESS(status) equals
                    TRUE if successful,

    STATUS_UNSUCCESSFUL, or another status for which NT_SUCCESS(status) equals
                    FALSE otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint(("DriverEntry: VJoyUM\n"));

    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Error: WdfDriverCreate failed 0x%x\n", status));
        return status;
    }

    return status;
}

static NTSTATUS
EvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDFDEVICE device;
    DEVICE_CONTEXT* deviceContext;
    PHID_DEVICE_ATTRIBUTES hidAttributes;
    UNREFERENCED_PARAMETER(Driver);

    KdPrint(("Enter EvtDeviceAdd\n"));

    //
    // Mark ourselves as a filter, which also relinquishes power policy ownership
    //
    WdfFdoInitSetFilter(DeviceInit);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &deviceAttributes,
        DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit,
                             &deviceAttributes,
                             &device);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Error: WdfDeviceCreate failed 0x%x\n", status));
        return status;
    }

    deviceContext = GetDeviceContext(device);
    deviceContext->device = device;

    hidAttributes = &deviceContext->hidDeviceAttributes;
    RtlZeroMemory(hidAttributes, sizeof(HID_DEVICE_ATTRIBUTES));
    hidAttributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
    hidAttributes->VendorID = HIDVJOY_VID;
    hidAttributes->ProductID = HIDVJOY_PID;
    hidAttributes->VersionNumber = HIDVJOY_VERSION;

    status = QueueCreate(device, &deviceContext->defaultQueue);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = ManualQueueCreate(
        device,
        EvtTimer,
        &deviceContext->pendingReadQueue);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    deviceContext->hidDescriptor = HidDescriptor;
    deviceContext->reportDescriptor = ReportDescriptor;
    return STATUS_SUCCESS;
}

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

NTSTATUS QueueCreate(
    _In_ WDFDEVICE Device,
    _Out_ WDFQUEUE* Queue
)
{
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES queueAttributes;
    WDFQUEUE queue;
    PQUEUE_CONTEXT queueContext;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchParallel);

    //
    // HIDclass uses INTERNAL_IOCTL which is not supported by UMDF. Therefore
    // the hidumdf.sys changes the IOCTL type to DEVICE_CONTROL for next stack
    // and sends it down
    //
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &queueAttributes,
        QUEUE_CONTEXT);

    status = WdfIoQueueCreate(
        Device,
        &queueConfig,
        &queueAttributes,
        &queue);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n",status));
        return status;
    }

    queueContext = GetQueueContext(queue);
    queueContext->queue = queue;
    queueContext->deviceContext = GetDeviceContext(Device);

    *Queue = queue;
    return status;
}

VOID EvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    NTSTATUS status;
    BOOLEAN completeRequest = TRUE;
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    DEVICE_CONTEXT* deviceContext = NULL;
    PQUEUE_CONTEXT queueContext = GetQueueContext(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    deviceContext = GetDeviceContext(device);

    switch (IoControlCode)
    {
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR: // METHOD_NEITHER
        KdPrint(("IOCTL_HID_GET_DEVICE_DESCRIPTOR\n"));
        _Analysis_assume_(deviceContext->HidDescriptor.bLength != 0);
        status = RequestCopyFromBuffer(Request,
                                       &deviceContext->hidDescriptor,
                                       deviceContext->hidDescriptor.bLength);
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES: // METHOD_NEITHER
        KdPrint(("IOCTL_HID_GET_DEVICE_ATTRIBUTES\n"));
        status = RequestCopyFromBuffer(Request,
                                       &queueContext->deviceContext->hidDeviceAttributes,
                                       sizeof(HID_DEVICE_ATTRIBUTES));
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR: // METHOD_NEITHER
        KdPrint(("IOCTL_HID_GET_REPORT_DESCRIPTOR\n"));
        status = RequestCopyFromBuffer(Request,
                                       deviceContext->reportDescriptor,
                                       deviceContext->hidDescriptor.DescriptorList[0].wReportLength);
        break;

    case IOCTL_HID_GET_STRING: // METHOD_NEITHER
        KdPrint(("IOCTL_HID_GET_STRING\n"));
        status = GetString(Request);
        break;


    case IOCTL_HID_READ_REPORT: // METHOD_NEITHER
        KdPrint(("IOCTL_HID_READ_REPORT\n"));
        status = ReadReport(queueContext, Request, &completeRequest);
        break;

    case IOCTL_HID_WRITE_REPORT: // METHOD_NEITHER
        KdPrintEx((T_TRACE, "IOCTL_HID_WRITE_REPORT\n"));
        status = WriteReport(queueContext, Request);
        break;

    case IOCTL_HID_GET_INDEXED_STRING: // METHOD_OUT_DIRECT
        KdPrint(("IOCTL_HID_GET_INDEXED_STRING\n"));
        status = GetIndexedString(Request);
        break;

    default:
        KdPrint(("Unsupported IoControlCode: %d\n", IoControlCode));
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (completeRequest)
    {
        WdfRequestComplete(Request, status);
    }
}


static NTSTATUS ReadReport(
    _In_ PQUEUE_CONTEXT QueueContext,
    _In_ WDFREQUEST Request,
    _Always_(_Out_)
    BOOLEAN* CompleteRequest
)
{
    NTSTATUS status = WdfRequestForwardToIoQueue(
        Request,
        QueueContext->deviceContext->pendingReadQueue);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfRequestForwardToIoQueue failed with 0x%x\n", status));
        *CompleteRequest = TRUE;
    }
    else
    {
        *CompleteRequest = FALSE;
    }

    return status;
}


static NTSTATUS WriteReport(
    _In_ PQUEUE_CONTEXT queueContext,
    _In_ WDFREQUEST request
)
{
    HID_XFER_PACKET transferPacket;
    VENDOR_DEVICE_PACKET* updatePacket = NULL;
    NTSTATUS status;
    ULONG bytesWritten = 0;

    status = RequestGetHidXferPacket_ToWriteToDevice(
        request,
        &transferPacket);
    if (!NT_SUCCESS(status))
    {
        KdPrint((
            "WriteReport: RequestGetHidXferPacket_ToWriteToDevice failed 0x%X\n",
            status
        ));
        return status;
    }


    if (transferPacket.reportId != REPORTID_VENDOR)
    {
        KdPrintEx((T_ERROR, "WriteReport: Unhandled report %d\n", transferPacket.reportId));
        return STATUS_INVALID_PARAMETER;
    }

    if (transferPacket.reportBufferLen != sizeof(VENDOR_DEVICE_PACKET))
    {
        KdPrintEx((
            T_ERROR,
            "WriteReport: Report of invalid size - %ld != %lld\n",
            transferPacket.reportBufferLen,
            sizeof(VENDOR_DEVICE_PACKET)
        ));
        return STATUS_INVALID_PARAMETER;
    }

    updatePacket = (VENDOR_DEVICE_PACKET*)transferPacket.reportBuffer;

    DUMP_VENDOR_DEVICE_REPORT(updatePacket);

    for (UINT i = 0; i < 2; ++i)
    {
        switch (queueContext->deviceContext->lastServedReportID)
        {
        default:
        case REPORTID_KEYBOARD:
            status = HandlePendingJoystickReadReportRequest(
                queueContext->deviceContext->pendingReadQueue,
                &updatePacket->joystick,
                &bytesWritten);
            if (NT_SUCCESS(status))
            {
                queueContext->deviceContext->lastServedReportID = REPORTID_JOYSTICK;
            }
            else
            {
                return status;
            }
            break;

        case REPORTID_JOYSTICK:
            status = HandlePendingKeyboardReadReportRequest(
                queueContext->deviceContext->pendingReadQueue,
                &updatePacket->keyboard,
                &bytesWritten);
            if (NT_SUCCESS(status))
            {
                queueContext->deviceContext->lastServedReportID = REPORTID_KEYBOARD;
            }
            else
            {
                return status;
            }
            break;
        }

        WdfRequestSetInformation(request, bytesWritten);
    }


    return status;
}

static NTSTATUS DequeuePendingReadRequest(_In_ WDFQUEUE queue, _In_ WDFREQUEST* reportRequest)
{
    NTSTATUS status = WdfIoQueueRetrieveNextRequest(queue, reportRequest);
    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_NO_MORE_ENTRIES || status == STATUS_WDF_PAUSED)
        {
            return status;
        }

        KdPrintEx((
            T_WARNING,
            "WriteReport: DequeuePendingReadRequest status %ld (0x%X)\n",
            status,
            status));
        return status;
    }
    return status;
}

static NTSTATUS HandlePendingJoystickReadReportRequest(
    _In_ WDFQUEUE queue,
    _In_ JOYSTICK_SUBREPORT* joystickState,
    _Out_ ULONG* bytesWritten
)
{
    NTSTATUS status;
    JOYSTICK_REPORT* outputReportBuffer;
    size_t outputBytesAvailable = 0;
    size_t reportLen = sizeof(*outputReportBuffer);
    WDFREQUEST request;

    KdPrintEx((T_TRACE, "HandlePendingJoystickReadReportRequest\n"));

    status = DequeuePendingReadRequest(queue, &request);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfRequestRetrieveOutputBuffer(
        request,
        reportLen,
        &outputReportBuffer,
        &outputBytesAvailable);
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((
            T_ERROR,
            "HandlePendingJoystickReadReportRequest: WdfRequestRetrieveOutputBuffer failed: 0x%X\n",
            status));
        return status;
    }

    if (outputBytesAvailable < reportLen)
    {
        KdPrintEx((
            T_ERROR,
            "HandlePendingJoystickReadReportRequest: output buffer size too small: %llu < %llu\n",
            outputBytesAvailable,
            reportLen));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(&outputReportBuffer->data, joystickState, sizeof(outputReportBuffer->data));
    outputReportBuffer->id = REPORTID_JOYSTICK;

    *bytesWritten = (ULONG)reportLen;
    WdfRequestCompleteWithInformation(request, status, *bytesWritten);

    return STATUS_SUCCESS;
}

static NTSTATUS HandlePendingKeyboardReadReportRequest(
    _In_ WDFQUEUE queue,
    _In_ KEYBOARD_SUBREPORT* keyboardState,
    _Out_ ULONG* bytesWritten
)
{
    NTSTATUS status;
    KEYBOARD_REPORT* outputReportBuffer;
    PMANUAL_QUEUE_CONTEXT queueContext;
    WDFREQUEST request;
    size_t outputBytesAvailable = 0;
    size_t reportLen = sizeof(*outputReportBuffer);

    KdPrintEx((T_TRACE, "HandlePendingKeyboardReadReportRequest\n"));

    status = DequeuePendingReadRequest(queue, &request);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfRequestRetrieveOutputBuffer(
        request,
        reportLen,
        &outputReportBuffer,
        &outputBytesAvailable);
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((
            T_ERROR,
            "HandlePendingKeyboardReadReportRequest: WdfRequestRetrieveOutputBuffer failed: 0x%X\n",
            status));
        return status;
    }

    if (outputBytesAvailable < reportLen)
    {
        KdPrintEx((
            T_ERROR,
            "HandlePendingKeyboardReadReportRequest: output buffer size too small: %llu < %llu\n",
            outputBytesAvailable,
            reportLen));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(&outputReportBuffer->data, keyboardState, sizeof(outputReportBuffer->data));
    outputReportBuffer->id = REPORTID_KEYBOARD;

    *bytesWritten = (ULONG)reportLen;
    WdfRequestCompleteWithInformation(request, status, *bytesWritten);

    queueContext = GetManualQueueContext(queue);
    WdfTimerStart(queueContext->timer, WDF_REL_TIMEOUT_IN_MS(FEEDER_DISCONNECT_TIMEOUT));

    return STATUS_SUCCESS;
}


static NTSTATUS GetStringId(
    _In_ WDFREQUEST Request,
    _Out_ ULONG* StringId,
    _Out_ ULONG* LanguageId
)
{
    NTSTATUS status;
    ULONG inputValue;

    WDFMEMORY inputMemory;
    size_t inputBufferLength;
    PVOID inputBuffer;

    //
    // mshidumdf.sys updates the IRP and passes the string id (or index) through
    // the input buffer correctly based on the IOCTL buffer type
    //

    status = WdfRequestRetrieveInputMemory(Request, &inputMemory);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfRequestRetrieveInputMemory failed 0x%x\n", status));
        return status;
    }
    inputBuffer = WdfMemoryGetBuffer(inputMemory, &inputBufferLength);

    //
    // make sure buffer is big enough.
    //
    if (inputBufferLength < sizeof(ULONG))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        KdPrint(("GetStringId: invalid input buffer. size %d, expect %d\n",
            (int)inputBufferLength, (int)sizeof(ULONG)));
        return status;
    }

    inputValue = (*(PULONG)inputBuffer);

    //
    // The least significant two bytes of the INT value contain the string id.
    //
    *StringId = (inputValue & 0x0ffff);

    //
    // The most significant two bytes of the INT value contain the language
    // ID (for example, a value of 1033 indicates English).
    //
    *LanguageId = (inputValue >> 16);

    return status;
}


static NTSTATUS GetIndexedString(_In_ WDFREQUEST Request)
{
    NTSTATUS status;
    ULONG languageId, stringIndex;

    status = GetStringId(Request, &stringIndex, &languageId);
    UNREFERENCED_PARAMETER(languageId);

    if (NT_SUCCESS(status))
    {
        if (stringIndex != VJOYUM_DEVICE_STRING_INDEX)
        {
            status = STATUS_INVALID_PARAMETER;
            KdPrint(("GetString: unkown string index %d\n", stringIndex));
            return status;
        }

        status = RequestCopyFromBuffer(
            Request,
            VJOYUM_DEVICE_STRING,
            sizeof(VJOYUM_DEVICE_STRING));
    }
    return status;
}


static NTSTATUS GetString(_In_ WDFREQUEST Request)
{
    NTSTATUS status;
    ULONG languageId, stringId;
    size_t stringSizeCb;
    PWSTR string;

    status = GetStringId(Request, &stringId, &languageId);

    // While we don't use the language id, some minidrivers might.
    //
    UNREFERENCED_PARAMETER(languageId);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    switch (stringId)
    {
    case HID_STRING_ID_IMANUFACTURER:
        stringSizeCb = sizeof(VJOYUM_MANUFACTURER_STRING);
        string = VJOYUM_MANUFACTURER_STRING;
        break;
    case HID_STRING_ID_IPRODUCT:
        stringSizeCb = sizeof(VJOYUM_PRODUCT_STRING);
        string = VJOYUM_PRODUCT_STRING;
        break;
    case HID_STRING_ID_ISERIALNUMBER:
        stringSizeCb = sizeof(VJOYUM_SERIAL_NUMBER_STRING);
        string = VJOYUM_SERIAL_NUMBER_STRING;
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        KdPrint(("GetString: unkown string id %d\n", stringId));
        return status;
    }

    status = RequestCopyFromBuffer(Request, string, stringSizeCb);
    return status;
}

static void EvtTimer(_In_ WDFTIMER timer)
{
    NTSTATUS status;
    WDFQUEUE queue;
    PMANUAL_QUEUE_CONTEXT queueContext;
    KEYBOARD_SUBREPORT emptyReport;
    ULONG bytesWritten;

    KdPrint(("EvtTimerFunc\n"));

    queue = (WDFQUEUE)WdfTimerGetParentObject(timer);
    queueContext = GetManualQueueContext(queue);

    memset(&emptyReport, 0, sizeof(emptyReport));

    status = HandlePendingKeyboardReadReportRequest(
        queueContext->deviceContext->pendingReadQueue,
        &emptyReport,
        &bytesWritten);
    if (NT_SUCCESS(status))
    {
        queueContext->deviceContext->lastServedReportID = REPORTID_KEYBOARD;
        WdfTimerStop(queueContext->timer, FALSE);
    }
    else
    {
        // Schedule another attempt.
        WdfTimerStart(queueContext->timer, WDF_REL_TIMEOUT_IN_MS(FEEDER_DISCONNECT_TIMEOUT));
    }
}
