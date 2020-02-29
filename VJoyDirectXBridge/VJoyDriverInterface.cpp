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
typedef std::map<HANDLE, CDriverInterface> HandleMap_t;
static HandleMap_t g_driverHandles;


//= F U N C T I O N S =========================================================================

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule,
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


//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API
HANDLE AttachToDriver(void)
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

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API
BOOL BeginDriverUpdateLoop(HANDLE attachID)
{
    HandleMap_t::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    return it->second.RunUpdateThread();
}

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API
BOOL EndDriverUpdateLoop(HANDLE attachID)
{
    HandleMap_t::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    return it->second.ExitUpdateThread();
}


//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API
BOOL DetachFromDriver(HANDLE attachID)
{
    HandleMap_t::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    BOOL ret = it->second.ExitUpdateThread();
    it->second.CloseDriverHandle();
    g_driverHandles.erase(it);

    return ret;
}


//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API
BOOL EnumerateDevices(HANDLE attachID, DeviceEnumCB callbackFunct)
{
    HandleMap_t::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    return it->second.EnumerateDevices(callbackFunct);
}


//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API
BOOL SetDeviceIDs(HANDLE attachID, const char* joystickGUIDStr, const char* rudderGUIDStr)
{
    HandleMap_t::iterator it = g_driverHandles.find(attachID);
    if (it == g_driverHandles.end())
        return FALSE;

    CDriverInterface::DeviceIDMapping mapping;

#define MAP( db, di, sb, si )  m.destBlock = db, m.destIndex = di, m.srcBlock = sb, m.srcIndex = si, m.invert = FALSE
#define MAPAXIS( di, si ) m.destBlock = m.srcBlock = CJoystickDevice::DS_AXIS, m.destIndex = di, m.srcIndex = si, m.invert = FALSE
#define MAPBUTTON( index )  m.destBlock = m.srcBlock = CJoystickDevice::DS_BUTTON, m.destIndex = m.srcIndex = index, m.invert = FALSE

    {
        GUID joystickGUID;
        ParseGUID(joystickGUID, joystickGUIDStr);
        CJoystickDevice::DeviceMappingVector mappingVector;

        CJoystickDevice::DeviceMapping m;

        // Xbox controller
        MAPAXIS(CJoystickDevice::axis_x, CJoystickDevice::axis_x);
        mappingVector.push_back(m);

        MAPAXIS(CJoystickDevice::axis_y, CJoystickDevice::axis_y);
        mappingVector.push_back(m);

        MAPAXIS(CJoystickDevice::axis_throttle, CJoystickDevice::axis_throttle);
        mappingVector.push_back(m);

        MAPAXIS(CJoystickDevice::axis_rx, CJoystickDevice::axis_rx);
        mappingVector.push_back(m);

        MAPAXIS(CJoystickDevice::axis_ry, CJoystickDevice::axis_ry);
        mappingVector.push_back(m);

        // Map buttons
        for (UINT32 i = 0; i < 12; ++i)
        {
            MAPBUTTON(i);
            mappingVector.push_back(m);
        }

        // Map our POV's
#ifdef HATS_AS_BUTTONS
                // Clear the old button mappings
                MAPBUTTON(HAT_1_AS_BUTTONS_OFFSET);
                dev.ClearMapping(m);
                MAPBUTTON(HAT_1_AS_BUTTONS_OFFSET + 1);
                dev.ClearMapping(m);
                MAPBUTTON(HAT_1_AS_BUTTONS_OFFSET + 2);
                dev.ClearMapping(m);
                MAPBUTTON(HAT_1_AS_BUTTONS_OFFSET + 3);
                dev.ClearMapping(m);
        
                MAP(CJoystickDevice::DS_BUTTON, HAT_1_AS_BUTTONS_OFFSET, CJoystickDevice::DS_POV, 0);
                mappingVector.push_back(m);
#else
        MAP(CJoystickDevice::DS_POV, 0, CJoystickDevice::DS_POV, 0);
        mappingVector.push_back(m);
#endif


        mapping[joystickGUID] = mappingVector;
    }

    if (strcmp(joystickGUIDStr, rudderGUIDStr))
    {
        GUID rudderGUID;
        ParseGUID(rudderGUID, rudderGUIDStr);
        CJoystickDevice::DeviceMappingVector mappingVector;

        CJoystickDevice::DeviceMapping m;
        MAPAXIS(CJoystickDevice::axis_rx, CJoystickDevice::axis_x); // X axis is left pedal (inverted)
        mappingVector.push_back(m);
        
        MAPAXIS(CJoystickDevice::axis_ry, CJoystickDevice::axis_y); // Y axis is right pedal (inverted)
        mappingVector.push_back(m);
        
        MAPAXIS(CJoystickDevice::axis_rz, CJoystickDevice::axis_rz); // RZ axis is rudder
        mappingVector.push_back(m);


        mapping[rudderGUID] = mappingVector;
    }

    return it->second.SetDeviceMapping(mapping);
}
