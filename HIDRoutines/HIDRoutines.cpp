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
