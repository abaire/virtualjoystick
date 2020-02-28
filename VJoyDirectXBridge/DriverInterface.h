/*!
 * File:      DriverInterface.h
 * Created:   2006/10/07 18:11
 * \file      c:\WinDDK\5600\src\hid\vjoy\VJoyDriverInterface\DriverInterface.h
 * \brief
 */

#pragma once

//= I N C L U D E S ===========================================================================
#include <dinput.h>
#include <string>
#include <vector>
#include <map>

#include "JoystickDevice.h"


//= C L A S S E S =============================================================================
//! \class	CDriverInterface
//! \brief      
class CDriverInterface
{
public:

    struct GUIDComparator
    {
        bool operator()(const GUID& l, const GUID& r) const
        {
#define CHECK_PARAM(p) if (l.p > r.p) { return FALSE; } if (l.p < r.p) { return TRUE; }
            CHECK_PARAM(Data1)
            CHECK_PARAM(Data2)
            CHECK_PARAM(Data3)
            CHECK_PARAM(Data4[0])
            CHECK_PARAM(Data4[1])
            CHECK_PARAM(Data4[2])
            CHECK_PARAM(Data4[3])
            CHECK_PARAM(Data4[4])
            CHECK_PARAM(Data4[5])
            CHECK_PARAM(Data4[6])
            CHECK_PARAM(Data4[7])
            return FALSE;
        }
    };

    typedef void (CALLBACK* DeviceEnumCB)(const char* name, const char* guid);
    typedef std::map<GUID, CJoystickDevice::DeviceMappingVector, GUIDComparator> DeviceIDMapping;

protected:
    typedef std::vector<CJoystickDevice> DeviceVector;

public:

    CDriverInterface(const std::string& deviceName);
    CDriverInterface();

    inline BOOL SetDeviceMapping(const DeviceIDMapping& deviceMap)
    {
        m_deviceGUIDMapping = deviceMap;
        return TRUE;
    }

    BOOL EnumerateDevices(DeviceEnumCB cb);

    BOOL RunUpdateThread(void);
    BOOL ExitUpdateThread(void);

    HANDLE& DriverHandle(void) { return m_driverHandle; }
    const HANDLE& DriverHandle(void) const { return m_driverHandle; }

    void CloseDriverHandle(void);


protected:

    DWORD UpdateThreadProc(void);

    static DWORD WINAPI UpdateThreadProc(LPVOID driverInterface)
    {
        return ((CDriverInterface*)driverInterface)->UpdateThreadProc();
    }

    //-------------------------------------------------------------------------------------------
    //	EnumJoysticksCB
    //! \brief		Callback for the joystick enumeration.  This callback finds our registered joystick and rudder device and
    //!           creates the physical JoystickDevice wrapper instances with appropriate mappings
    //-------------------------------------------------------------------------------------------
    BOOL EnumJoysticksCB(const DIDEVICEINSTANCE* inst);

    static BOOL CALLBACK EnumJoysticksCB(const DIDEVICEINSTANCE* inst, VOID* pContext)
    {
        return ((CDriverInterface*)pContext)->EnumJoysticksCB(inst);
    }

    BOOL OpenDirectXHandle(void);

protected:
    std::string m_deviceName;
    HANDLE m_driverHandle;

    HANDLE m_updateThreadHandle;

    volatile BOOL m_updateThreadRunning;

    DWORD m_updateLoopDelay;

    static LPDIRECTINPUT8 m_pDI;
    DeviceVector m_inputDeviceVector;
    DeviceIDMapping m_deviceGUIDMapping;
};
