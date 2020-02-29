/*!
* File:      VJoyDriverInterface.cpp
* Created:   2006/10/07 17:09
* \file      c:\WinDDK\5600\src\hid\vjoy\VJoyDriverInterface\VJoyDriverInterface.cpp
* \brief
*/


//= I N C L U D E S ===========================================================================
#include "stdafx.h"
#include <map>

#include "VJoyDriverInterface.h"
#include "HIDRoutines.h"
#include "DriverInterface.h"

//= G L O B A L = V A R S =====================================================================
typedef std::map<HANDLE, CDriverInterface> HandleMap;
static HandleMap g_driverHandles;

static inline void ParseGUID(GUID& ret, const char* str);


//= F U N C T I O N S =========================================================================


BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


extern "C"
VJOYDRIVERINTERFACE_API
HANDLE AttachToVirtualJoystickDriver(void)
{
    // Find our driver instance
    std::string deviceName;

    char* joystickNameBuffer = NULL;
    if (!FindJoystickDeviceName(&joystickNameBuffer) || !joystickNameBuffer)
        return INVALID_HANDLE_VALUE;

    deviceName = joystickNameBuffer;
    delete[] joystickNameBuffer;

    // Create a driver connection struct
    CDriverInterface handle(deviceName);

    // Ensure that we don't already have a mapping for this handle
    if (g_driverHandles.find(handle.DriverHandle()) != g_driverHandles.end())
    {
        CloseHandle(handle.DriverHandle());
        return INVALID_HANDLE_VALUE;
    }

    g_driverHandles[handle.DriverHandle()] = handle;
    return handle.DriverHandle();
}


extern "C"
VJOYDRIVERINTERFACE_API
BOOL BeginDriverUpdateLoop(HANDLE attachID)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    return it->second.RunUpdateThread();
}


extern "C"
VJOYDRIVERINTERFACE_API
BOOL EndDriverUpdateLoop(HANDLE attachID)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    return it->second.ExitUpdateThread();
}


extern "C"
VJOYDRIVERINTERFACE_API
BOOL DetachFromVirtualJoystickDriver(HANDLE attachID)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    BOOL ret = it->second.ExitUpdateThread();
    it->second.CloseDriverHandle();
    g_driverHandles.erase(it);

    return ret;
}


extern "C"
VJOYDRIVERINTERFACE_API
BOOL EnumerateDevices(HANDLE attachID, DeviceEnumCB callbackFunc)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    return it->second.EnumerateDevices(callbackFunc);
}

extern "C"
VJOYDRIVERINTERFACE_API
BOOL GetDeviceInfo(
    HANDLE attachID,
    _In_ const char* deviceGUIDStr,
    _Out_ UINT32* numAxes,
    _Out_ UINT32* numButtons,
    _Out_ UINT32* numPOVs)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    GUID deviceGUID;
    ParseGUID(deviceGUID, deviceGUIDStr);
    return it->second.GetDeviceInfo(deviceGUID, *numAxes, *numButtons, *numPOVs);
}

extern "C"
VJOYDRIVERINTERFACE_API
BOOL SetDeviceMapping(HANDLE attachID, const char* deviceGUIDStr, const DeviceMapping* mappings, size_t mappingCount)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    CJoystickDevice::DeviceMappingVector mappingVector(mappings, mappings + mappingCount);
    GUID guid;
    ParseGUID(guid, deviceGUIDStr);

    return it->second.AddDeviceMapping(guid, mappingVector);
}

extern "C"
VJOYDRIVERINTERFACE_API
BOOL ClearDeviceMappings(HANDLE attachID)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;
    return it->second.ClearDeviceMappings();
}


static inline void ParseGUID(GUID& ret, const char* str)
{
    UINT32 val1, val2, val3, val4_1, val4_2, val4_3, val4_4, val4_5, val4_6, val4_7, val4_8;
    sscanf_s(
        str,
        "%8X%4X%4X%2X%2X%2X%2X%2X%2X%2X%2X",
        &val1,
        &val2,
        &val3,
        &val4_1,
        &val4_2,
        &val4_3,
        &val4_4,
        &val4_5,
        &val4_6,
        &val4_7,
        &val4_8);

    ret.Data1 = val1;
    ret.Data2 = val2;
    ret.Data3 = val3;
    ret.Data4[0] = val4_1;
    ret.Data4[1] = val4_2;
    ret.Data4[2] = val4_3;
    ret.Data4[3] = val4_4;
    ret.Data4[4] = val4_5;
    ret.Data4[5] = val4_6;
    ret.Data4[6] = val4_7;
    ret.Data4[7] = val4_8;
}
