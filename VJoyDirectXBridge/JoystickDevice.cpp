/*!
 * File:      JoystickDevice.cpp
 * Created:   2006/10/07 18:38
 * \file      c:\WinDDK\5600\src\hid\vjoy\VJoyDriverInterface\JoystickDevice.cpp
 * \brief     
 */

//= I N C L U D E S ===========================================================================
#include "stdafx.h"
#include "JoystickDevice.h"


//= P R O T O T Y P E S =======================================================================
static BOOL CALLBACK EnumJoysticksCB( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );

INT_PTR CJoystickDevice::JOYSTATEAXISOFFSETS[8];
INT_PTR CJoystickDevice::DEVPACKETAXISOFFSETS[8];

//= F U N C T I O N S =========================================================================

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
CJoystickDevice::CJoystickDevice( void ) :
  m_handle( NULL )
, m_deviceInstance()
{

}

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
CJoystickDevice::CJoystickDevice( LPDIRECTINPUT8 di, const DIDEVICEINSTANCE &device ) :
  m_deviceInstance( device )
{
  HRESULT hr = di->CreateDevice( m_deviceInstance.guidInstance, &m_handle, NULL );


    // Create the joystate offset maps
  {
    DIJOYSTATE2 tmp;
    INT_PTR base = (INT_PTR)&tmp;
    JOYSTATEAXISOFFSETS[AXIS_X] = (INT_PTR)&tmp.lX - base;
    JOYSTATEAXISOFFSETS[AXIS_Y] = (INT_PTR)&tmp.lY - base;
    JOYSTATEAXISOFFSETS[AXIS_Z] = (INT_PTR)&tmp.lZ - base;
    JOYSTATEAXISOFFSETS[AXIS_RX] = (INT_PTR)&tmp.lRx - base;
    JOYSTATEAXISOFFSETS[AXIS_RY] = (INT_PTR)&tmp.lRy - base;
    JOYSTATEAXISOFFSETS[AXIS_RZ] = (INT_PTR)&tmp.lRz - base;
    JOYSTATEAXISOFFSETS[AXIS_S0] = (INT_PTR)&tmp.rglSlider[0] - base;
    JOYSTATEAXISOFFSETS[AXIS_S1] = (INT_PTR)&tmp.rglSlider[1] - base;
  }

    // Create the dev packet offset maps
  {
    DEVICE_PACKET tmp;
    INT_PTR base = (INT_PTR)&tmp;
    DEVPACKETAXISOFFSETS[AXIS_X] = (INT_PTR)&tmp.report.X - base;
    DEVPACKETAXISOFFSETS[AXIS_Y] = (INT_PTR)&tmp.report.Y - base;
    DEVPACKETAXISOFFSETS[AXIS_Z] = (INT_PTR)&tmp.report.Z - base;
    DEVPACKETAXISOFFSETS[AXIS_RX] = (INT_PTR)&tmp.report.rX - base;
    DEVPACKETAXISOFFSETS[AXIS_RY] = (INT_PTR)&tmp.report.rY - base;
    DEVPACKETAXISOFFSETS[AXIS_RZ] = (INT_PTR)&tmp.report.rZ - base;
    DEVPACKETAXISOFFSETS[AXIS_S0] = (INT_PTR)&tmp.report.Slider[0] - base;
    DEVPACKETAXISOFFSETS[AXIS_S1] = (INT_PTR)&tmp.report.Slider[1] - base;
  }
}

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
BOOL CJoystickDevice::Acquire( void )
{
  if( !m_handle || FAILED( m_handle->SetDataFormat( &c_dfDIJoystick2 ) ) )
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
  if( FAILED( m_handle->EnumObjects( EnumObjectsCallback, 
                                     (VOID*)this, 
                                     DIDFT_ALL ) ) )
  {
    //PRINTMSG(( "Failed to enum stick objects!" ));
    return FALSE;
  }

  return TRUE;
}

//-------------------------------------------------------------------------------------------
//	EnumObjectsCallback
//-------------------------------------------------------------------------------------------
BOOL CJoystickDevice::EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE *obj )
{
    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values.
  if( obj->dwType & DIDFT_AXIS )
  {
    DIPROPRANGE diprg; 
    diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
    diprg.diph.dwHow        = DIPH_BYID; 
    diprg.diph.dwObj        = obj->dwType;
    diprg.lMin              = AXIS_MIN; 
    diprg.lMax              = AXIS_MAX; 

    // Set the range for the axis
    HRESULT hr;
    if( FAILED( hr = m_handle->SetProperty( DIPROP_RANGE, &diprg.diph ) ) ) 
    {
      //hr = m_handle->GetProperty( DIPROP_RANGE, &diprg.diph );
      //PRINTMSG(( T_ERROR, "Failed to set range property on joystick axis!\n" ));
      return DIENUM_STOP;
    }
  }

  return DIENUM_CONTINUE;
}



//-------------------------------------------------------------------------------------------
//  AddMapping
//-------------------------------------------------------------------------------------------
BOOL CJoystickDevice::AddMapping( const DeviceMapping &srcMap ) 
{ 
    // Validate the mapping, for now axes may only be driven by axes, POV's by POV's, and Buttons by Buttons or POV's
  if( srcMap.destBlock != srcMap.srcBlock )
  {
    if( srcMap.destBlock != DS_BUTTON || 
        (srcMap.srcBlock != DS_POV && srcMap.srcBlock != DS_BUTTON) )
      return FALSE;
  }

  ClearMapping( srcMap );
  m_deviceMapping.push_back( srcMap ); 

  return TRUE;
}


//-------------------------------------------------------------------------------------------
//  ClearMapping
//-------------------------------------------------------------------------------------------
void CJoystickDevice::ClearMapping( const DeviceMapping &map )
{
  DeviceMappingVector::iterator it = m_deviceMapping.begin();
  DeviceMappingVector::iterator itEnd = m_deviceMapping.end();
  for( ; it != itEnd; ++it )
  {
    if( it->destBlock == map.destBlock &&
        it->destIndex == map.destIndex &&
        it->srcBlock == map.srcBlock &&
        it->srcIndex == map.srcIndex )
    {
      m_deviceMapping.erase( it );
      return;
    }
  }
}
