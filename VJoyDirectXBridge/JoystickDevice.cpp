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
{
    HRESULT hr = di->CreateDevice(m_deviceInstance.guidInstance, &m_handle, NULL);


    // Create the joystate offset maps
    {
        DIJOYSTATE2 tmp;
        INT_PTR base = (INT_PTR)&tmp;
        JOYSTATEAXISOFFSETS[axis_x] = (INT_PTR)&tmp.lX - base;
        JOYSTATEAXISOFFSETS[axis_y] = (INT_PTR)&tmp.lY - base;
        JOYSTATEAXISOFFSETS[axis_throttle] = (INT_PTR)&tmp.lZ - base;
        JOYSTATEAXISOFFSETS[axis_rx] = (INT_PTR)&tmp.lRx - base;
        JOYSTATEAXISOFFSETS[axis_ry] = (INT_PTR)&tmp.lRy - base;
        JOYSTATEAXISOFFSETS[axis_rz] = (INT_PTR)&tmp.lRz - base;
        JOYSTATEAXISOFFSETS[axis_slider] = (INT_PTR)&tmp.rglSlider[0] - base;
        JOYSTATEAXISOFFSETS[axis_dial] = (INT_PTR)&tmp.rglSlider[1] - base;
    }
}

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
BOOL CJoystickDevice::Acquire(void)
{
    if (!m_handle || FAILED(m_handle->SetDataFormat( &c_dfDIJoystick2 )))
    {
        //PRINTMSG(( "Failed to set stick data format!\n" ));
        return FALSE;
    }

    /*
    if( FAILED( stick->SetCooperativeLevel( GetConsoleWindow(), DISCL_EXCLUSIVE | DISCL_BACKGROUND ) ) )
    {
      printf( "Failed to set stick coop level!\n" );
      return;
    }
  */

    // Enumerate the joystick objects and set the min/max values property for any discovered axes.
    if (FAILED(m_handle->EnumObjects( EnumObjectsCallback,
        (VOID*)this,
        DIDFT_ALL )))
    {
        //PRINTMSG(( "Failed to enum stick objects!" ));
        return FALSE;
    }

    return TRUE;
}

//-------------------------------------------------------------------------------------------
//	EnumObjectsCallback
//-------------------------------------------------------------------------------------------
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
        if (FAILED(hr = m_handle->SetProperty( DIPROP_RANGE, &diprg.diph )))
        {
            //hr = m_handle->GetProperty( DIPROP_RANGE, &diprg.diph );
            //PRINTMSG(( T_ERROR, "Failed to set range property on joystick axis!\n" ));
            return DIENUM_STOP;
        }
    }

    return DIENUM_CONTINUE;
}
