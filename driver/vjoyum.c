#include "vjoyum.h"
#include "HIDReportDescriptor.h"

#if DBG
#define T_ERROR         DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL
#define T_WARNING       DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL
#define T_TRACE         DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL
#define T_INFO          DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL
#else
#define T_ERROR
#define T_WARNING
#define T_TRACE
#define T_INFO
#endif


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

NTSTATUS
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

    //
    // Use default "HID Descriptor" (hardcoded). We will set the
    // wReportLength memeber of HID descriptor when we read the
    // the report descriptor either from registry or the hard-coded
    // one.
    //
    deviceContext->HidDescriptor = HidDescriptor;
    deviceContext->ReportDescriptor = ReportDescriptor;
    KdPrint(("Using Hard-coded Report descriptor\n"));
    return STATUS_SUCCESS;
}

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

NTSTATUS
QueueCreate(
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
    queueContext->OutputReport = 0;

    *Queue = queue;
    return status;
}

VOID
EvtIoDeviceControl(
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

    KdPrintEx((T_ERROR, "EvtIoDeviceControl: %ld 0x%X\n", IoControlCode, IoControlCode));

    switch (IoControlCode)
    {
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR: // METHOD_NEITHER
        _Analysis_assume_(deviceContext->HidDescriptor.bLength != 0);
        status = RequestCopyFromBuffer(Request,
                                       &deviceContext->HidDescriptor,
                                       deviceContext->HidDescriptor.bLength);
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES: // METHOD_NEITHER
        status = RequestCopyFromBuffer(Request,
                                       &queueContext->DeviceContext->HidDeviceAttributes,
                                       sizeof(HID_DEVICE_ATTRIBUTES));
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR: // METHOD_NEITHER
        status = RequestCopyFromBuffer(Request,
                                       deviceContext->ReportDescriptor,
                                       deviceContext->HidDescriptor.DescriptorList[0].wReportLength);
        break;

    case IOCTL_HID_GET_STRING: // METHOD_NEITHER

        status = GetString(Request);
        break;


    case IOCTL_HID_READ_REPORT: // METHOD_NEITHER
    case IOCTL_UMDF_HID_GET_INPUT_REPORT:
        //
        // Returns a report from the device into a class driver-supplied
        // buffer.
        //
        status = ReadReport(queueContext, Request, &completeRequest);
        break;

    case IOCTL_HID_WRITE_REPORT: // METHOD_NEITHER
    case IOCTL_UMDF_HID_SET_OUTPUT_REPORT: // METHOD_NEITHER
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
                KdPrint((
                    "WriteReport: Report of invalid size - %ld != %d\n",
                    transferPacket.reportBufferLen,
                    sizeof(DEVICE_PACKET)
                ));
                return STATUS_INVALID_PARAMETER;
            }

            WDFREQUEST readReportReq;
            updatePacket = (PDEVICE_PACKET)transferPacket.reportBuffer;
            
                      #define DUMP_DEVICE_REPORT( _v_ ) \
                        KdPrintEx(( T_ERROR, \
                                    "\tX: 0x%X\n" \
                                      "\tY: 0x%X\n" \
                                      "\tZ: 0x%X\n" \
                                      "\trX: 0x%X\n" \
                                      "\trY: 0x%X\n" \
                                      "\trZ: 0x%X\n" \
                                      "\tDial[0]: 0x%X\n" \
                                      "\tDial[1]: 0x%X\n" \
                                      "\tButton[0]: 0x%X\n" \
                                      "\tButton[1]: 0x%X\n" \
                                      "\tButton[2]: 0x%X\n" \
                                      "\tButton[3]: 0x%X\n" \
                                      "\tButton[4]: 0x%X\n" \
                                      "\tButton[5]: 0x%X\n" \
                                      , ((PDEVICE_REPORT)(_v_))->X \
                                      , ((PDEVICE_REPORT)(_v_))->Y \
                                      , ((PDEVICE_REPORT)(_v_))->Z \
                                      , ((PDEVICE_REPORT)(_v_))->rX \
                                      , ((PDEVICE_REPORT)(_v_))->rY \
                                      , ((PDEVICE_REPORT)(_v_))->rZ \
                                      , ((PDEVICE_REPORT)(_v_))->Slider[0] \
                                      , ((PDEVICE_REPORT)(_v_))->Slider[1] \
                                      , ((PDEVICE_REPORT)(_v_))->Button[0] \
                                      , ((PDEVICE_REPORT)(_v_))->Button[1] \
                                      , ((PDEVICE_REPORT)(_v_))->Button[2] \
                                      , ((PDEVICE_REPORT)(_v_))->Button[3] \
                                      , ((PDEVICE_REPORT)(_v_))->Button[4] \
                                      , ((PDEVICE_REPORT)(_v_))->Button[5] ))
    
                      DUMP_DEVICE_REPORT( &updatePacket->report );
          
            // See if there's a valid request pending a response
            status = WdfIoQueueRetrieveNextRequest(QueueContext->DeviceContext->ManualQueue, &readReportReq);
            if (!NT_SUCCESS(status))
            {
                KdPrintEx((T_ERROR, "WriteReport: WdfIoQueueRetrieveNextRequest status %ld (0x%X)\n", status, status));
                return status;
            }
            ULONG bytesWritten;

            //updatePacket->id = REPORTID_JOYSTICK;
            ServiceJoystickReadReportRequest(readReportReq, updatePacket, &bytesWritten);

            // Report back to the caller that we've written bytes
            WdfRequestSetInformation(Request, bytesWritten);
        }
        break;

    default:
        KdPrintEx((T_ERROR, "WriteReport: Unhandled report %d\n", transferPacket.reportId));
        return STATUS_INVALID_PARAMETER;
    }

    return status;
}


//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
static NTSTATUS ServiceJoystickReadReportRequest(
    _In_ WDFREQUEST readReportReq,
    _In_ PDEVICE_PACKET pPacket,
    _Out_ ULONG* bytesWritten
)
{
    PDEVICE_PACKET outputReportBuffer;
    size_t outputBytesAvailable = 0;
    size_t reportLen = sizeof(*pPacket);
    NTSTATUS status;

    *bytesWritten = 0;

    // Grab the output buffer so we can copy our report into it and complete it
    status = WdfRequestRetrieveOutputBuffer(readReportReq,
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

    KdPrintEx((T_ERROR, "Output bytes avail: %ld", outputBytesAvailable));

    // Copy the report into the output buffer and complete the outstanding read request
    RtlCopyMemory(outputReportBuffer, pPacket, reportLen);
    outputReportBuffer->id = REPORTID_JOYSTICK;

    WdfRequestCompleteWithInformation(readReportReq, status, reportLen);

    *bytesWritten = (ULONG)reportLen;

    return STATUS_SUCCESS;
}

NTSTATUS
GetStringId(
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
        KdPrint(("WdfRequestRetrieveInputMemory failed 0x%x\n",status));
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


NTSTATUS
GetIndexedString(
    _In_ WDFREQUEST Request
)
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

    // While we don't use the language id, some minidrivers might.
    //
    UNREFERENCED_PARAMETER(languageId);

    if (NT_SUCCESS(status))
    {
        if (stringIndex != VHIDMINI_DEVICE_STRING_INDEX)
        {
            status = STATUS_INVALID_PARAMETER;
            KdPrint(("GetString: unkown string index %d\n", stringIndex));
            return status;
        }

        status = RequestCopyFromBuffer(Request, VHIDMINI_DEVICE_STRING, sizeof(VHIDMINI_DEVICE_STRING));
    }
    return status;
}


NTSTATUS
GetString(
    _In_ WDFREQUEST Request
)
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
        stringSizeCb = sizeof(VHIDMINI_MANUFACTURER_STRING);
        string = VHIDMINI_MANUFACTURER_STRING;
        break;
    case HID_STRING_ID_IPRODUCT:
        stringSizeCb = sizeof(VHIDMINI_PRODUCT_STRING);
        string = VHIDMINI_PRODUCT_STRING;
        break;
    case HID_STRING_ID_ISERIALNUMBER:
        stringSizeCb = sizeof(VHIDMINI_SERIAL_NUMBER_STRING);
        string = VHIDMINI_SERIAL_NUMBER_STRING;
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        KdPrint(("GetString: unkown string id %d\n", stringId));
        return status;
    }

    status = RequestCopyFromBuffer(Request, string, stringSizeCb);
    return status;
}


NTSTATUS
ManualQueueCreate(
    _In_ WDFDEVICE Device,
    _Out_ WDFQUEUE* Queue
)
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
        KdPrint(("WdfIoQueueCreate failed 0x%x\n",status));
        return status;
    }

    queueContext = GetManualQueueContext(queue);
    queueContext->Queue = queue;
    queueContext->DeviceContext = GetDeviceContext(Device);

    *Queue = queue;

    return status;
}

// void
// EvtTimerFunc(
//     _In_ WDFTIMER Timer
// )
// /*++
// Routine Description:
//
//     This periodic timer callback routine checks the device's manual queue and
//     completes any pending request with data from the device.
//
// Arguments:
//
//     Timer - Handle to a timer object that was obtained from WdfTimerCreate.
//
// Return Value:
//
//     VOID
//
// --*/
// {
//     NTSTATUS status;
//     WDFQUEUE queue;
//     PMANUAL_QUEUE_CONTEXT queueContext;
//     WDFREQUEST request;
//     HIDMINI_INPUT_REPORT readReport;
//
//     KdPrint(("EvtTimerFunc\n"));
//
//     queue = (WDFQUEUE)WdfTimerGetParentObject(Timer);
//     queueContext = GetManualQueueContext(queue);
//
//     //
//     // see if we have a request in manual queue
//     //
//     status = WdfIoQueueRetrieveNextRequest(
//         queueContext->Queue,
//         &request);
//
//     if (NT_SUCCESS(status))
//     {
//         readReport.ReportId = REPORTID_VENDOR;
//         readReport.Data = queueContext->DeviceContext->DeviceData;
//
//         status = RequestCopyFromBuffer(request,
//                                        &readReport,
//                                        sizeof(readReport));
//
//         WdfRequestComplete(request, status);
//     }
// }
