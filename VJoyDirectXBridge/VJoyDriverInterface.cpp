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
    CDriverInterface driverInterface(deviceName);

    // Ensure that we don't already have a mapping for this handle
    if (g_driverHandles.find(driverInterface.DriverHandle()) != g_driverHandles.end())
    {
        CloseHandle(driverInterface.DriverHandle());
        return INVALID_HANDLE_VALUE;
    }

    auto driverHandle = driverInterface.DriverHandle();
    g_driverHandles[driverHandle] = std::move(driverInterface);
    return driverHandle;
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
BOOL GetDeviceInfo(HANDLE attachID, _In_ const char* deviceGUIDStr, DeviceInfoCB callbackFunc)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    GUID deviceGUID;
    ParseGUID(deviceGUID, deviceGUIDStr);

    CDriverInterface::DeviceObjectInfoVector axes;
    CDriverInterface::DeviceObjectInfoVector buttons;
    CDriverInterface::DeviceObjectInfoVector povs;

    if (!it->second.GetDeviceInfo(deviceGUID, axes, buttons, povs))
    {
        return FALSE;
    }

    for each (auto info in axes)
    {
        callbackFunc(mt_axis, info.first.c_str(), info.second);
    }

    for each (auto info in buttons)
    {
        callbackFunc(mt_button, info.first.c_str(), info.second);
    }

    for each (auto info in povs)
    {
        callbackFunc(mt_pov, info.first.c_str(), info.second);
    }

    return TRUE;
}

extern "C"
VJOYDRIVERINTERFACE_API
BOOL SetDeviceMapping(HANDLE attachID, const char* deviceGUIDStr, const DeviceMapping* mappings, size_t mappingCount)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    CJoystickDevice::DeviceMappingVector mappingVector(mappingCount);
    for (size_t i = 0; i < mappingCount; ++i)
    {
        auto mapping = *mappings++;
        if (mapping.destIndex != UNMAPPED_INDEX)
        {
            mappingVector.push_back(mapping);
        }
    }
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

extern "C"
VJOYDRIVERINTERFACE_API
UINT32 UpdateLoopDelay(HANDLE attachID)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;
    return it->second.UpdateLoopDelay();
}

extern "C"
VJOYDRIVERINTERFACE_API
BOOL SetUpdateLoopDelay(HANDLE attachID, UINT32 delay)
{
    HandleMap::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;
    it->second.SetUpdateLoopDelay(delay);
    return TRUE;
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
