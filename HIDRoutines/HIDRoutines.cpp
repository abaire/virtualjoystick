/*!
 * File:      HIDRoutines.cpp
 * Created:   2006/10/07 17:18
 * \file      HIDRoutines.cpp
 * \brief
 */
 //= I N C L U D E S ===========================================================================
#include "stdafx.h"
#include <stdio.h>


//= P R O T O T Y P E S =======================================================================
static BOOL CheckDeviceAttributes(const HDEVINFO& hardwareDeviceInfo, PSP_DEVICE_INTERFACE_DATA devData, TCHAR** ret);


//= F U N C T I O N S =========================================================================

//-------------------------------------------------------------------------------------------
//  FindJoystickDeviceName
//-------------------------------------------------------------------------------------------
BOOL FindJoystickDeviceName(TCHAR** ret)
{
    // Grab our hid GUID
    GUID hidguid;
    HidD_GetHidGuid(&hidguid);

    // See if we can get the class devices
    HDEVINFO hardwareDeviceInfo = SetupDiGetClassDevs((LPGUID)&hidguid,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (hardwareDeviceInfo == INVALID_HANDLE_VALUE)
    {
        //printf("SetupDiGetClassDevs failed: %x\n", GetLastError());
        return FALSE;
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // See if we can find an instance of the virtual joystick driver
    BOOL devFound = FALSE;
    for (UINT32 i = 0; SetupDiEnumDeviceInterfaces(hardwareDeviceInfo, 0, (LPGUID)&hidguid, i, &deviceInterfaceData); ++i)
    {
        if (CheckDeviceAttributes(hardwareDeviceInfo, &deviceInterfaceData, ret))
        {
            devFound = TRUE;
            break;
        }
    }

    // Clean up
    SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

    return devFound;
}

//-------------------------------------------------------------------------------------------
//	CheckDeviceAttributes
//-------------------------------------------------------------------------------------------
static BOOL CheckDeviceAttributes(const HDEVINFO& hardwareDeviceInfo,
    PSP_DEVICE_INTERFACE_DATA deviceInterfaceData,
    TCHAR** ret)
{
    // Figure out how much room we need
    DWORD requiredLength;
    SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
        deviceInterfaceData,
        NULL,
        0,
        &requiredLength,
        NULL);

    DWORD predictedLength = requiredLength;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredLength);
    if (!deviceInterfaceDetailData)
    {
        //printf("Error: CheckDeviceAttributes: malloc failed\n");
        return FALSE;
    }

    deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    // Get our device info
    if (!SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
        deviceInterfaceData,
        deviceInterfaceDetailData,
        predictedLength,
        &requiredLength,
        NULL))
    {
        //printf("Error: SetupDiGetInterfaceDeviceDetail failed\n");
        free(deviceInterfaceDetailData);
        return FALSE;
    }

    HANDLE file = CreateFile(deviceInterfaceDetailData->DevicePath,
        0,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        //printf( "Error: CreateFile failed: %d\n", GetLastError() );
        free(deviceInterfaceDetailData);

        return FALSE;
    }

    // See if the device is a fake joystick by checking its attribs
    HIDD_ATTRIBUTES attr;
    attr.Size = sizeof(attr);
    if (!HidD_GetAttributes(file, &attr) ||
        attr.VendorID != HIDVJOY_VID ||
        attr.ProductID != HIDVJOY_PID)
    {
        CloseHandle(file);
        free(deviceInterfaceDetailData);
        return FALSE;
    }

    // Now we know it's related to our root virtual driver, make sure this is the
    // vendor report handle
    PHIDP_PREPARSED_DATA preparsedData = NULL;
    HIDP_CAPS caps;

    if (!HidD_GetPreparsedData(file, &preparsedData))
    {
        CloseHandle(file);
        free(deviceInterfaceDetailData);
        return FALSE;
    }

    if (!HidP_GetCaps(preparsedData, &caps))
    {
        HidD_FreePreparsedData(preparsedData);
        CloseHandle(file);
        free(deviceInterfaceDetailData);
        return FALSE;
    }


    USAGE vendorDefined = 0xff00;
    USAGE vendorUsage1 = 0x0001;
    if (caps.UsagePage != vendorDefined || caps.Usage != vendorUsage1)
    {
        HidD_FreePreparsedData(preparsedData);
        CloseHandle(file);
        free(deviceInterfaceDetailData);
        return FALSE;
    }

    CloseHandle(file);


    *ret = _tcsdup(deviceInterfaceDetailData->DevicePath);
    free(deviceInterfaceDetailData);

    if (!*ret)
        return FALSE;

    return TRUE;
}

// Debugging stuff no longer needed, the driver should be accessed via WriteFile
#if 0
//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
BOOL SendJoystickDevicePacket(HANDLE driverHandle, const PDEVICE_PACKET devicePacket)
{
    OutputDebugString("!!!!!!!!!!!!!!!!\nChecking to ensure driver is what we expect\n");
    PHIDP_PREPARSED_DATA Ppd = NULL; // The opaque parser info describing this device
    HIDD_ATTRIBUTES                 Attributes; // The Attributes of this hid device.
    HIDP_CAPS                       Caps; // The Capabilities of this hid device.
    BOOLEAN                         result = FALSE;

    if (!HidD_GetPreparsedData(driverHandle, &Ppd))
        OutputDebugString("Error: HidD_GetPreparsedData failed \n");
    if (!HidD_GetAttributes(driverHandle, &Attributes))
        OutputDebugString("Error: HidD_GetAttributes failed \n");

    if (Attributes.VendorID == HIDVJOY_VID && Attributes.ProductID == HIDVJOY_PID)
    {
        if (!HidP_GetCaps(Ppd, &Caps))
        {
            OutputDebugString("Error: HidP_GetCaps failed \n");
        }

        TCHAR msgBuf[1024];
        _stprintf(msgBuf,
            "UsagePage: %X, Usage: %X, FeatureReportLen: %d, OutputReportLen: %d, InputReportLen: %d\n",
            Caps.UsagePage,
            Caps.Usage,
            Caps.FeatureReportByteLength,
            Caps.OutputReportByteLength,
            Caps.InputReportByteLength);
        OutputDebugString(msgBuf);
    }
    else
    {
        TCHAR msgBuf[1024];
        _stprintf(msgBuf,
            "VendorID/ProductID mismatch: %X %X != %X %X\n",
            Attributes.VendorID,
            Attributes.ProductID,
            HIDVJOY_VID,
            HIDVJOY_PID);
        OutputDebugString(msgBuf);
    }
    HidD_FreePreparsedData(Ppd);


    char buffer[4096] = { 0 };
    BOOLEAN blah;
    TCHAR msgBuf[1024];

    buffer[0] = REPORTID_VENDOR;
    /*
      blah = HidD_GetFeature( driverHandle, buffer, 37 );
      _stprintf( msgBuf, "HidD_GetFeature size %d: %d\n", 37, blah );
      OutputDebugString( msgBuf );

      blah = HidD_GetInputReport( driverHandle, buffer, 37 );
      _stprintf( msgBuf, "HidD_GetInputReport: %d\n", blah );
      OutputDebugString( msgBuf );


      blah = HidD_SetOutputReport( driverHandle, (PVOID)devicePacket, sizeof(*devicePacket) );
      _stprintf( msgBuf, "HidD_SetOutputReport: %d\n", blah );
      OutputDebugString( msgBuf );
    */

    _stprintf(msgBuf, "Buffer info: Ptr: %p   size: %d, first byte: %d\n", devicePacket, sizeof(*devicePacket), *(UCHAR*)devicePacket);
    OutputDebugString(msgBuf);


    DWORD bytesWritten;
    if (!WriteFile(driverHandle,
        devicePacket,
        sizeof(*devicePacket),
        &bytesWritten,
        NULL))
    {
        _stprintf(msgBuf, "WriteFile failed: %d\n", GetLastError());
        OutputDebugString(msgBuf);
        /*
        DWORD WINAPI FormatMessage(
        __in      DWORD dwFlags,
        __in_opt  LPCVOID lpSource,
        __in      DWORD dwMessageId,
        __in      DWORD dwLanguageId,
        __out     LPTSTR lpBuffer,
        __in      DWORD nSize,
        __in_opt  va_list *Arguments
        );
        */
    }
    else
    {
        _stprintf(msgBuf, "WriteFile ok, wrote: %d bytes\n", bytesWritten);
        OutputDebugString(msgBuf);
    }

    /*
      buffer[0] = 0;
      for( int i = 30; i < 50; ++i )
      {
        blah = HidD_SetFeature( driverHandle, buffer, i );
        _stprintf( msgBuf, "HidD_SetFeature size %d: %d\n", i, blah );
        OutputDebugString( msgBuf );

        blah = HidD_SetOutputReport( driverHandle, buffer, i );
        _stprintf( msgBuf, "HidD_SetOutputReport size %d: %d\n", i, blah );
        OutputDebugString( msgBuf );
      }
    /*
      buffer[0] = REPORTID_JOYSTICK;
      for( int i = 30; i < 50; ++i )
      {
        blah = HidD_SetFeature( driverHandle, buffer, i );
        _stprintf( msgBuf, "HidD_SetFeature size %d: %d\n", i, blah );
        OutputDebugString( msgBuf );
      }
    */


    OutputDebugString("^^^^^^^^^^^^^^\n");


    return HidD_SetFeature(driverHandle, (PVOID)devicePacket, sizeof(*devicePacket));
}
#endif
