#include "vjoyum.h"
#include "common.h"
#include "util.h"
#include "HIDReportDescriptor.h"

#define T_ERROR         DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL
#define T_WARNING       DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL
#define T_TRACE         DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL
#define T_INFO          DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL

#define DUMP_DEVICE_REPORT( _v_ ) KdPrintEx(( \
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
    , ((PDEVICE_REPORT)(_v_))->X \
    , ((PDEVICE_REPORT)(_v_))->Y \
    , ((PDEVICE_REPORT)(_v_))->Throttle \
    , ((PDEVICE_REPORT)(_v_))->Rudder \
    , ((PDEVICE_REPORT)(_v_))->rX \
    , ((PDEVICE_REPORT)(_v_))->rY \
    , ((PDEVICE_REPORT)(_v_))->rZ \
    , ((PDEVICE_REPORT)(_v_))->Slider \
    , ((PDEVICE_REPORT)(_v_))->Dial \
    , ((PDEVICE_REPORT)(_v_))->POV \
    , ((PDEVICE_REPORT)(_v_))->Button[0] \
    , ((PDEVICE_REPORT)(_v_))->Button[1] \
    , ((PDEVICE_REPORT)(_v_))->Button[2] \
    , ((PDEVICE_REPORT)(_v_))->Button[3] \
    , ((PDEVICE_REPORT)(_v_))->Button[4] \
    , ((PDEVICE_REPORT)(_v_))->Button[4] \
    , ((PDEVICE_REPORT)(_v_))->Button[5] \
    , ((PDEVICE_REPORT)(_v_))->Button[6] \
    , ((PDEVICE_REPORT)(_v_))->Button[7] \
    , ((PDEVICE_REPORT)(_v_))->Button[8] \
    , ((PDEVICE_REPORT)(_v_))->Button[10] \
    , ((PDEVICE_REPORT)(_v_))->Button[11] \
    , ((PDEVICE_REPORT)(_v_))->Button[12] \
    , ((PDEVICE_REPORT)(_v_))->Button[13] \
    , ((PDEVICE_REPORT)(_v_))->Button[14] \
    , ((PDEVICE_REPORT)(_v_))->Button[15] ))

static EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;

static NTSTATUS ServiceJoystickReadReportRequest(
    _In_ WDFREQUEST readReportReq,
    _In_ PDEVICE_PACKET pPacket,
    _Out_ ULONG* bytesWritten
);

static NTSTATUS ReadReport(
    _In_ PQUEUE_CONTEXT QueueContext,
    _In_ WDFREQUEST Request,
    _Always_(_Out_)
    BOOLEAN* CompleteRequest
);

static NTSTATUS WriteReport(
    _In_ PQUEUE_CONTEXT QueueContext,
    _In_ WDFREQUEST Request
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

    KdPrint(("DriverEntry for VHidMini\n"));

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
    PDEVICE_CONTEXT deviceContext;
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
    deviceContext->Device = device;
    deviceContext->DeviceData = 0;

    hidAttributes = &deviceContext->HidDeviceAttributes;
    RtlZeroMemory(hidAttributes, sizeof(HID_DEVICE_ATTRIBUTES));
    hidAttributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
    hidAttributes->VendorID = HIDVJOY_VID;
    hidAttributes->ProductID = HIDVJOY_PID;
    hidAttributes->VersionNumber = HIDVJOY_VERSION;

    status = QueueCreate(device, &deviceContext->DefaultQueue);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = ManualQueueCreate(device, &deviceContext->ManualQueue);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    deviceContext->HidDescriptor = HidDescriptor;
    deviceContext->ReportDescriptor = ReportDescriptor;
    return STATUS_SUCCESS;
}

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

NTSTATUS QueueCreate(
    _In_ WDFDEVICE Device,
    _Out_ WDFQUEUE* Queue
)
/*++
Routine Description:

    This function creates a default, parallel I/O queue to proces IOCTLs
    from hidclass.sys.

Arguments:

    Device - Handle to a framework device object.

    Queue - Output pointer to a framework I/O queue handle, on success.

Return Value:

    NTSTATUS

--*/
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
    queueContext->Queue = queue;
    queueContext->DeviceContext = GetDeviceContext(Device);

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
/*++
Routine Description:

    This event callback function is called when the driver receives an

    (KMDF) IOCTL_HID_Xxx code when handlng IRP_MJ_INTERNAL_DEVICE_CONTROL
    (UMDF) IOCTL_HID_Xxx, IOCTL_UMDF_HID_Xxx when handling IRP_MJ_DEVICE_CONTROL

Arguments:

    Queue - A handle to the queue object that is associated with the I/O request

    Request - A handle to a framework request object.

    OutputBufferLength - The length, in bytes, of the request's output buffer,
            if an output buffer is available.

    InputBufferLength - The length, in bytes, of the request's input buffer, if
            an input buffer is available.

    IoControlCode - The driver or system defined IOCTL associated with the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    BOOLEAN completeRequest = TRUE;
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT deviceContext = NULL;
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
                                       &deviceContext->HidDescriptor,
                                       deviceContext->HidDescriptor.bLength);
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES: // METHOD_NEITHER
        KdPrint(("IOCTL_HID_GET_DEVICE_ATTRIBUTES\n"));
        status = RequestCopyFromBuffer(Request,
                                       &queueContext->DeviceContext->HidDeviceAttributes,
                                       sizeof(HID_DEVICE_ATTRIBUTES));
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR: // METHOD_NEITHER
        KdPrint(("IOCTL_HID_GET_REPORT_DESCRIPTOR\n"));
        status = RequestCopyFromBuffer(Request,
                                       deviceContext->ReportDescriptor,
                                       deviceContext->HidDescriptor.DescriptorList[0].wReportLength);
        break;

    case IOCTL_HID_GET_STRING: // METHOD_NEITHER
        KdPrint(("IOCTL_HID_GET_STRING\n"));
        status = GetString(Request);
        break;


    case IOCTL_HID_READ_REPORT: // METHOD_NEITHER
    case IOCTL_UMDF_HID_GET_INPUT_REPORT:
        KdPrint(("IOCTL_HID_READ_REPORT IOCTL_UMDF_HID_GET_INPUT_REPORT\n"));
        //
        // Returns a report from the device into a class driver-supplied
        // buffer.
        //
        status = ReadReport(queueContext, Request, &completeRequest);
        break;

    case IOCTL_HID_WRITE_REPORT: // METHOD_NEITHER
    case IOCTL_UMDF_HID_SET_OUTPUT_REPORT: // METHOD_NEITHER
        KdPrint(("IOCTL_HID_WRITE_REPORT IOCTL_UMDF_HID_SET_OUTPUT_REPORT\n"));
        //
        // Transmits a class driver-supplied report to the device.
        //
        status = WriteReport(queueContext, Request);
        break;

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

    case IOCTL_HID_GET_INDEXED_STRING: // METHOD_OUT_DIRECT
        KdPrint(("IOCTL_HID_GET_INDEXED_STRING\n"));
        status = GetIndexedString(Request);
        break;

    default:
        KdPrint(("Unsupported IoControlCode: %d\n", IoControlCode));
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    //
    // Complete the request. Information value has already been set by request
    // handlers.
    //
    if (completeRequest)
    {
        WdfRequestComplete(Request, status);
    }
}

NTSTATUS
RequestCopyFromBuffer(
    _In_ WDFREQUEST Request,
    _In_ PVOID SourceBuffer,
    _When_(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    _In_ size_t NumBytesToCopyFrom
)
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
{
    NTSTATUS status;
    WDFMEMORY memory;
    size_t outputBufferLength;

    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfRequestRetrieveOutputMemory failed 0x%x\n",status));
        return status;
    }

    WdfMemoryGetBuffer(memory, &outputBufferLength);
    if (outputBufferLength < NumBytesToCopyFrom)
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        KdPrint(("RequestCopyFromBuffer: buffer too small. Size %d, expect %d\n",
            (int)outputBufferLength, (int)NumBytesToCopyFrom));
        return status;
    }

    status = WdfMemoryCopyFromBuffer(memory,
                                     0,
                                     SourceBuffer,
                                     NumBytesToCopyFrom);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfMemoryCopyFromBuffer failed 0x%x\n",status));
        return status;
    }

    WdfRequestSetInformation(Request, NumBytesToCopyFrom);
    return status;
}

NTSTATUS
ReadReport(
    _In_ PQUEUE_CONTEXT QueueContext,
    _In_ WDFREQUEST Request,
    _Always_(_Out_)
    BOOLEAN* CompleteRequest
)
{
    NTSTATUS status;

    KdPrint(("ReadReport!\n"));

    //
    // forward the request to manual queue
    //
    status = WdfRequestForwardToIoQueue(
        Request,
        QueueContext->DeviceContext->ManualQueue);
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

NTSTATUS
WriteReport(
    _In_ PQUEUE_CONTEXT QueueContext,
    _In_ WDFREQUEST Request
)
/*++

Routine Description:

    Handles IOCTL_HID_SET_OUTPUT_REPORT

Arguments:

    QueueContext - The object context associated with the queue

    Request - Pointer to Request Packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    HID_XFER_PACKET transferPacket;

    status = RequestGetHidXferPacket_ToWriteToDevice(
        Request,
        &transferPacket);
    if (!NT_SUCCESS(status))
    {
        KdPrint((
            "WriteReport: RequestGetHidXferPacket_ToWriteToDevice failed 0x%X\n",
            status
        ));
        return status;
    }

    PDEVICE_PACKET updatePacket = NULL;

    switch (transferPacket.reportId)
    {
    case REPORTID_VENDOR:
        {
            // Pull out the report data - it should be the size of our DEVICE_REPORT plus
            //  1 byte for the reportID
            if (transferPacket.reportBufferLen != sizeof(DEVICE_PACKET))
            {
                KdPrintEx((
                    T_ERROR,
                    "WriteReport: Report of invalid size - %ld != %lld\n",
                    transferPacket.reportBufferLen,
                    sizeof(DEVICE_PACKET)
                ));
                return STATUS_INVALID_PARAMETER;
            }

            updatePacket = (PDEVICE_PACKET)transferPacket.reportBuffer;

            DUMP_DEVICE_REPORT(&updatePacket->report);

            // See if there's a valid request pending a response
            WDFREQUEST readReportReq;
            status = WdfIoQueueRetrieveNextRequest(QueueContext->DeviceContext->ManualQueue, &readReportReq);
            if (!NT_SUCCESS(status))
            {
                if (status == STATUS_NO_MORE_ENTRIES || status == STATUS_WDF_PAUSED)
                {
                    return STATUS_SUCCESS;
                }
                KdPrintEx((
                    T_WARNING, 
                    "WriteReport: WdfIoQueueRetrieveNextRequest status %ld (0x%X)\n", 
                    status,
                    status));
                return status;
            }

            ULONG bytesWritten;
            ServiceJoystickReadReportRequest(readReportReq, updatePacket, &bytesWritten);

            WdfRequestSetInformation(Request, bytesWritten);
        }
        break;

    default:
        KdPrintEx((T_ERROR, "WriteReport: Unhandled report %d\n", transferPacket.reportId));
        return STATUS_INVALID_PARAMETER;
    }

    return status;
}


NTSTATUS GetStringId(
    _In_ WDFREQUEST Request,
    _Out_ ULONG* StringId,
    _Out_ ULONG* LanguageId
)
/*++

Routine Description:

    Helper routine to decode IOCTL_HID_GET_INDEXED_STRING and IOCTL_HID_GET_STRING.

Arguments:

    Request - Pointer to Request Packet.

Return Value:

    NT status code.

--*/
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


NTSTATUS GetIndexedString(_In_ WDFREQUEST Request)
/*++

Routine Description:

    Handles IOCTL_HID_GET_INDEXED_STRING

Arguments:

    Request - Pointer to Request Packet.

Return Value:

    NT status code.

--*/
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

        status = RequestCopyFromBuffer(Request, VJOYUM_DEVICE_STRING, sizeof(VJOYUM_DEVICE_STRING));
    }
    return status;
}


NTSTATUS GetString(_In_ WDFREQUEST Request)
/*++

Routine Description:

    Handles IOCTL_HID_GET_STRING.

Arguments:

    Request - Pointer to Request Packet.

Return Value:

    NT status code.

--*/
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

static NTSTATUS ServiceJoystickReadReportRequest(
    _In_ WDFREQUEST readReportReq,
    _In_ PDEVICE_PACKET pPacket,
    _Out_ ULONG* bytesWritten
)
{
    PDEVICE_PACKET outputReportBuffer;
    size_t outputBytesAvailable = 0;
    size_t reportLen = sizeof(*pPacket);

    *bytesWritten = 0;

    NTSTATUS status;
    status = WdfRequestRetrieveOutputBuffer(
        readReportReq,
        reportLen,
        &outputReportBuffer,
        &outputBytesAvailable);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (outputBytesAvailable < reportLen)
    {
        KdPrintEx((T_ERROR, "ServiceJoystickReadReportRequest: output buffer size too small\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Copy the report into the output buffer and complete the outstanding read request
    RtlCopyMemory(outputReportBuffer, pPacket, reportLen);
    outputReportBuffer->id = REPORTID_JOYSTICK;

    WdfRequestCompleteWithInformation(readReportReq, status, reportLen);

    *bytesWritten = (ULONG)reportLen;

    return STATUS_SUCCESS;
}
