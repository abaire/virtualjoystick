/*!
 * File:      JoystickDevice.cpp
 * Created:   2006/10/07 18:38
 * \brief     
 */

//= I N C L U D E S ===========================================================================
#include "stdafx.h"
#include "JoystickDevice.h"


//= P R O T O T Y P E S =======================================================================
static BOOL CALLBACK EnumJoysticksCB(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext);

INT_PTR CJoystickDevice::JOYSTATEAXISOFFSETS[8];

//= F U N C T I O N S =========================================================================

CJoystickDevice::CJoystickDevice(
    LPDIRECTINPUT8 di,
    const DIDEVICEINSTANCE& device,
    const DeviceMappingVector& deviceMapping
) :
    m_deviceInstance(device)
    , m_deviceMapping(deviceMapping)
    , m_state()
    , m_isPolled(FALSE)
{
    HRESULT hr = di->CreateDevice(m_deviceInstance.guidInstance, &m_inputDevice, NULL);
    if (FAILED(hr))
    {
        m_inputDevice = NULL;
    }

    if (m_inputDevice)
    {
        DIDEVCAPS caps;
        caps.dwSize = sizeof(caps);
        if (!FAILED(m_inputDevice->GetCapabilities(&caps)))
        {
            m_isPolled = (caps.dwFlags & DIDC_POLLEDDATAFORMAT) != 0;
        }
    }

    // Create the joystate offset maps
    {
        DIJOYSTATE2 tmp;
        INT_PTR base = (INT_PTR)&tmp;
        JOYSTATEAXISOFFSETS[static_cast<UINT32>(AxisIndex::axis_x)] = (INT_PTR)&tmp.lX - base;
        JOYSTATEAXISOFFSETS[static_cast<UINT32>(AxisIndex::axis_y)] = (INT_PTR)&tmp.lY - base;
        JOYSTATEAXISOFFSETS[static_cast<UINT32>(AxisIndex::axis_throttle)] = (INT_PTR)&tmp.lZ - base;
        JOYSTATEAXISOFFSETS[static_cast<UINT32>(AxisIndex::axis_rx)] = (INT_PTR)&tmp.lRx - base;
        JOYSTATEAXISOFFSETS[static_cast<UINT32>(AxisIndex::axis_ry)] = (INT_PTR)&tmp.lRy - base;
        JOYSTATEAXISOFFSETS[static_cast<UINT32>(AxisIndex::axis_rz)] = (INT_PTR)&tmp.lRz - base;
        JOYSTATEAXISOFFSETS[static_cast<UINT32>(AxisIndex::axis_slider)] = (INT_PTR)&tmp.rglSlider[0] - base;
        JOYSTATEAXISOFFSETS[static_cast<UINT32>(AxisIndex::axis_dial)] = (INT_PTR)&tmp.rglSlider[1] - base;
    }

    memset(m_simulatedButtonPressDownMillisRemaining, 0, sizeof(m_simulatedButtonPressDownMillisRemaining));
    memset(m_simulatedButtonRepeatIntervalMillisRemaining, 0, sizeof(m_simulatedButtonRepeatIntervalMillisRemaining));
}

CJoystickDevice::CJoystickDevice(CJoystickDevice&& o) noexcept
{
    *this = std::move(o);
}

CJoystickDevice& CJoystickDevice::operator=(CJoystickDevice&& o) noexcept
{
    if (&o == this) { return *this; }

    m_deviceInstance = std::move(o.m_deviceInstance);
    m_inputDevice = std::exchange(o.m_inputDevice, (LPDIRECTINPUTDEVICE8)NULL);
    m_isPolled = o.m_isPolled;
    m_state = o.m_state;
    m_deviceMapping = std::move(o.m_deviceMapping);

    memcpy(m_simulatedButtonPressDownMillisRemaining, o.m_simulatedButtonPressDownMillisRemaining, sizeof(m_simulatedButtonPressDownMillisRemaining));
    memcpy(m_simulatedButtonRepeatIntervalMillisRemaining, o.m_simulatedButtonRepeatIntervalMillisRemaining, sizeof(m_simulatedButtonRepeatIntervalMillisRemaining));

    return *this;
}

HRESULT CJoystickDevice::Acquire(void)
{
    if (!m_inputDevice)
    {
        return E_HANDLE;
    }

    HRESULT result = m_inputDevice->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(result))
    {
        return result;
    }

    // Enumerate the joystick objects and set the min/max values property for any discovered axes.
    result = m_inputDevice->EnumObjects(
        EnumObjectsCallback,
        (VOID*)this,
        DIDFT_ALL);
    return result;
}

void CJoystickDevice::Unacquire(void)
{
    if (m_inputDevice)
    {
        m_inputDevice->Unacquire();
    }
}

BOOL CJoystickDevice::EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* obj)
{
    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values.
    if (obj->dwType & DIDFT_AXIS)
    {
        DIPROPRANGE diprg;
        diprg.diph.dwSize = sizeof(DIPROPRANGE);
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        diprg.diph.dwHow = DIPH_BYID;
        diprg.diph.dwObj = obj->dwType;
        diprg.lMin = AXIS_MIN;
        diprg.lMax = AXIS_MAX;

        // Set the range for the axis
        HRESULT hr;
        if (FAILED(hr = m_inputDevice->SetProperty( DIPROP_RANGE, &diprg.diph )))
        {
            //PRINTMSG(( T_ERROR, "Failed to set range property on joystick axis!\n" ));
            return DIENUM_STOP;
        }
    }

    return DIENUM_CONTINUE;
}
