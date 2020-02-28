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

#include "JoystickDevice.h"


//= C L A S S E S =============================================================================
 //! \class	CDriverInterface
 //! \brief      
class CDriverInterface
{
public:
    typedef void (CALLBACK* DeviceEnumCB)(const char* name, const char* guid);

protected:
    typedef std::vector<CJoystickDevice> DeviceVector;
    typedef std::vector<GUID> DeviceIDVector;

public:

    //-------------------------------------------------------------------------------------------
    //	
    //! \brief		
    //-------------------------------------------------------------------------------------------
    CDriverInterface(void);

    //-------------------------------------------------------------------------------------------
    //	
    //! \brief		
    //-------------------------------------------------------------------------------------------
    CDriverInterface(const std::string& deviceName);

    //-------------------------------------------------------------------------------------------
    //	
    //! \brief		
    //-------------------------------------------------------------------------------------------
    inline BOOL SetDevicesToRegister(const GUID& joystickGUID, const GUID& rudderGUID) {
        m_joystickGUID = joystickGUID;
        m_rudderGUID = rudderGUID;

        return TRUE;
    }

    //-------------------------------------------------------------------------------------------
    //	
    //-------------------------------------------------------------------------------------------
    BOOL EnumerateDevices(DeviceEnumCB cb);


    //-------------------------------------------------------------------------------------------
    //	
    //! \brief		
    //-------------------------------------------------------------------------------------------
    BOOL RunUpdateThread(void);

    //-------------------------------------------------------------------------------------------
    //	
    //! \brief		
    //-------------------------------------------------------------------------------------------
    BOOL ExitUpdateThread(void);


    std::string& DeviceName(void) { return m_deviceName; }
    const std::string& DeviceName(void) const { return m_deviceName; }

    HANDLE& DriverHandle(void) { return m_driverHandle; }
    const HANDLE& DriverHandle(void) const { return m_driverHandle; }

    void CloseDriverHandle(void);


protected:
    //-------------------------------------------------------------------------------------------
    //	
    //! \brief		
    //-------------------------------------------------------------------------------------------
    DWORD UpdateThreadProc(void);
    static DWORD WINAPI UpdateThreadProc(LPVOID driverInterface) { return ((CDriverInterface*)driverInterface)->UpdateThreadProc(); }

    //-------------------------------------------------------------------------------------------
    //	EnumJoysticksCB
    //! \brief		Callback for the joystick enumeration.  This callback finds our registered joystick and rudder device and
    //!           creates the physical JoystickDevice wrapper instances with appropriate mappings
    //-------------------------------------------------------------------------------------------
    BOOL EnumJoysticksCB(const DIDEVICEINSTANCE* inst);
    static BOOL CALLBACK EnumJoysticksCB(const DIDEVICEINSTANCE* inst, VOID* pContext) { return ((CDriverInterface*)pContext)->EnumJoysticksCB(inst); }

    BOOL OpenDirectXHandle(void);

protected:
    std::string             m_deviceName;
    HANDLE                  m_driverHandle;

    HANDLE                  m_updateThreadHandle;

    volatile BOOL           m_updateThreadRunning;

    DWORD                   m_updateLoopDelay;

    static LPDIRECTINPUT8   m_pDI;
    DeviceVector            m_inputDeviceVector;

    GUID                    m_joystickGUID;
    GUID                    m_rudderGUID;
};

