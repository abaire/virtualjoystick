/*!
 * File:      JoystickDevice.h
 * Created:   2006/10/07 18:37
 * \file      vjoy\VJoyDriverInterface\JoystickDevice.h
 * \brief     
 */


#pragma once

//= I N C L U D E S ===========================================================================
#include <dinput.h>
#include <vector>

//= D E F I N E S =============================================================================
#define MAX_ACQUIRE_RETRIES   512     //!< Sanity limit on the maximum number of attempts to aquire the physical device

#define AXIS_MIN  -32767
#define AXIS_MAX  32767


  // DirectInput reports values between 0 and 31500 (and -1 as a NULL val)
  // However the driver expects a value between 0 and 7 (or -1 for NULL)
  // It'd be nice to be able to use a full gradient, but DInput doesn't seem
  //  to support any values > 31500, so it adds no value.
#define MAP_RANGE( val )  (INT8)(val > 0 ? val / 4500 : val)

#define SETBUTTON( offset, val ) SetButton( packet, offset, val )


//= C L A S S E S =============================================================================
  //! \class  CJoystickDevice
  //! \brief  Wraps a phsyical input device and maps its state into the kernel virtual joystick driver
class CJoystickDevice
{
public:
    //! \enum   MappingType
    //! \brief  Defines the type of value to be mapped to the virtual driver
  enum MappingType {
    DS_AXIS = 0,
    DS_POV,
    DS_BUTTON
  };

    //! \enum   AxisIndex
    //! \brief  Defines the index of the various axes in the JOYSTATEAXISOFFSETS and DEVPACKETAXISOFFSETS arrays
  enum AxisIndex {
    AXIS_X = 0,
    AXIS_Y,
    AXIS_Z,
    AXIS_RX,
    AXIS_RY,
    AXIS_RZ,
    AXIS_S0,     // Slider 0
    AXIS_S1,     // Slider 1
  };


    //! \struct DeviceMapping
    //! \brief  Maps a specific dinput state variable to an output slot in the virtual joystick
  struct DeviceMapping
  {
    MappingType   destBlock;    //!< The type of the target virtual joystick state
    DWORD         destIndex;    //!< The index of the target virtual joystick state

    MappingType   srcBlock;     //!< The type of the source state
    DWORD         srcIndex;     //!< The index of the source joystick state

    BOOL          invert;       //!< Whether or not we should logically invert the physical state when injecting the virtual device
  };


public:
  typedef std::vector<DeviceMapping> DeviceMappingVector;

public:

    //-------------------------------------------------------------------------------------------
  	//	
  	//! \brief		
    //-------------------------------------------------------------------------------------------
  CJoystickDevice( void );

    //-------------------------------------------------------------------------------------------
  	//	
  	//! \brief		
    //-------------------------------------------------------------------------------------------
  CJoystickDevice( LPDIRECTINPUT8, const DIDEVICEINSTANCE &device );

    //-------------------------------------------------------------------------------------------
  	//	
  	//! \brief		
    //-------------------------------------------------------------------------------------------
  BOOL Acquire();

    //-------------------------------------------------------------------------------------------
  	//	
  	//! \brief		
    //-------------------------------------------------------------------------------------------
  inline void Release( void ) { if( m_handle ) m_handle->Release(); m_handle = NULL; }


    //-------------------------------------------------------------------------------------------
    //  AddMapping
    //! \brief  Registers a DeviceMapping instance that will be used to transform the physical
    //!         device state into the virtual device
    //!
    //! \return BOOL - TRUE if the mapping was accepted, FALSE if there was some conflict 
    //!               (typically an unsupported mapping, such as AXIS -> BUTTON)
    //-------------------------------------------------------------------------------------------
  BOOL AddMapping( const DeviceMapping &m );

    //-------------------------------------------------------------------------------------------
    //  ClearMapping
    //! \brief  Removes a device mapping that was previously set
    //-------------------------------------------------------------------------------------------
  void ClearMapping( const DeviceMapping &m );

    //-------------------------------------------------------------------------------------------
  	//	GetVirtualStateUpdatePacket
  	//! \brief		Polls the current physical device states and formats a HID DEVICE_PACKET message
    //!             that will be sent to the kernel miniport driver
    //!
    //! \warning  It is the responsibility of the caller to prevent any calls to Add/ClearMapping
    //!           while this function is being invoked!  Failure to do so may cause crashes 
    //!           or inconsistent state!
    //-------------------------------------------------------------------------------------------
  inline BOOL GetVirtualStateUpdatePacket( DEVICE_PACKET &packet );


protected:
    //-------------------------------------------------------------------------------------------
  	//	
  	//! \brief		
    //-------------------------------------------------------------------------------------------
  BOOL EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE * );
  static BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE *obj, VOID *context ) {
    return ((CJoystickDevice*)context)->EnumObjectsCallback( obj );
  }

    //-------------------------------------------------------------------------------------------
  	//	PollPhysicalStick
  	//! \brief		Polls for the current state of the physical device wrapped by this instance
    //-------------------------------------------------------------------------------------------
  inline BOOL PollPhysicalStick( void )
  {
    HRESULT hr = m_handle->Poll();
    if( FAILED( hr ) )  
    {
      UINT numRetries = 0;

      hr = m_handle->Acquire();
      while( hr == DIERR_INPUTLOST && numRetries++ < MAX_ACQUIRE_RETRIES ) 
        hr = m_handle->Acquire();
      return FALSE;
    }
    
    if( FAILED( hr = m_handle->GetDeviceState( sizeof(m_state), &m_state ) ) )
    {
      //printf( "ERROR checking dev state for joystick\n" );
      return FALSE;
    }  

    return TRUE;
  }


protected:
  DIDEVICEINSTANCE      m_deviceInstance;         //!< Information about the physical device being wrapped by this instance
  LPDIRECTINPUTDEVICE8  m_handle;                 //!< dinput handle to the physical device being wrapped by this instance

  DIJOYSTATE2           m_state;                  //!< The current state of the physical joystick represented by this instance
  static INT_PTR        JOYSTATEAXISOFFSETS[8];   //!< byte offset from the head of a DIJOYSTATE2 struct to the DWORD axis values
  static INT_PTR        DEVPACKETAXISOFFSETS[8];  //!< byte offset from the head of a DEVICE_PACKET struct to the UINT16 axis values

    //! Vector of mapping structure instance defining how to map the physical device state to the virtual multiplexed device
  DeviceMappingVector   m_deviceMapping;          
};












//-------------------------------------------------------------------------------------------
//  SetButton
//-------------------------------------------------------------------------------------------
static __inline void SetButton( DEVICE_PACKET &packet, UINT32 index, BOOL val )
{
  UINT32 offset = index / 8;
  UINT32 bit = 1 << (index & 0x07);

  if( val )
    packet.report.Button[offset] |= bit;
  else
    packet.report.Button[offset] &= ~bit;
}


//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
inline BOOL CJoystickDevice::GetVirtualStateUpdatePacket( DEVICE_PACKET &packet )
{
  if( !PollPhysicalStick() )
    return FALSE;

    // NOTE: There is a concurrency issue here that we're requiring the caller to handle. It should
    // not be permissible for the device mapping vector to be modified (via Add/Clear mapping) while
    // we're fetching data.
    // The fetch must be a fast operation, however, and we don't want to have to lock a sem per device per poll
  DeviceMappingVector::iterator it = m_deviceMapping.begin();
  DeviceMappingVector::iterator itEnd = m_deviceMapping.end();
  for( ; it != itEnd; ++it )
  {
    switch( it->destBlock )
    {
      // ** DS_AXIS ** //
    case DS_AXIS:
      {
          // JOYSTATEAXISOFFSETS holds the byte offset from the start of the joystate struct
          // to the various axis IDs.
          // DIJOYSTATE2 axes are held in 32 bit values, so we get a pointer to the proper address, cast
          // to UINT32 *, dereference as a UINT32, and then cast that value to an INT16 for return (we've
          // previously set the device up to return values in the 16 bit range)
        INT16 src = (INT16)*( (LONG*)(((BYTE*)&m_state) + JOYSTATEAXISOFFSETS[it->srcIndex]));
        if( it->invert )
          src = AXIS_MAX - src;
        *(INT16*)((BYTE*)&packet + DEVPACKETAXISOFFSETS[it->destIndex]) = src;
      }
      break;

      // ** DS_POV ** //
    case DS_POV:
      packet.report.POV[it->destIndex] = MAP_RANGE( m_state.rgdwPOV[it->srcIndex] );
      break;

      // ** DS_BUTTON ** //
    case DS_BUTTON:
      {
        switch( it->srcBlock )
        {
          // ** DS_BUTTON ** //
        case DS_BUTTON:
          SETBUTTON( it->destIndex, (m_state.rgbButtons[it->srcIndex] == 0x80) );
          break;

          // ** DS_POV ** //
        case DS_POV:
          {
            BOOL buttonSet[4] = {FALSE,FALSE,FALSE,FALSE};

            INT8 hatState = MAP_RANGE( m_state.rgdwPOV[it->srcIndex] );
            switch( hatState )
            {
            case 0:
              buttonSet[0] = TRUE;
              break;
            case 1:
              buttonSet[0] = buttonSet[1] = TRUE;
              break;
            case 2:
              buttonSet[1] = TRUE;
              break;
            case 3:
              buttonSet[1] = buttonSet[2] = TRUE;
              break;
            case 4:
              buttonSet[2] = TRUE;
              break;
            case 5:
              buttonSet[2] = buttonSet[3] = TRUE;
              break;
            case 6:
              buttonSet[3] = TRUE;
              break;
            case 7:
              buttonSet[3] = buttonSet[0] = TRUE;
              break;

            default:
              break;
            }

              // Apply the button set
            SETBUTTON( it->destIndex, buttonSet[0] );
            SETBUTTON( it->destIndex + 1, buttonSet[1] );
            SETBUTTON( it->destIndex + 2, buttonSet[2] );
            SETBUTTON( it->destIndex + 3, buttonSet[3] );
          }
          break;
        }
      }
    }
  }

  return TRUE;
}
