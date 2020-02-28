/*!
 * File:      VJoyDriverInterface.h
 * Created:   2006/10/07 17:11
 * \file      VJoyDriverInterface\VJoyDriverInterface.h
 * \brief     
 */

#pragma once

//= D E F I N E S =============================================================================
#ifdef VJOYDIRECTXBRIDGE_EXPORTS
# define VJOYDRIVERINTERFACE_API __declspec(dllexport)
#else
# define VJOYDRIVERINTERFACE_API __declspec(dllimport)
#endif

typedef void (CALLBACK *DeviceEnumCB)( const char *name, const char *guid );

//= P R O T O T Y P E S =======================================================================

  //-------------------------------------------------------------------------------------------
	//	AttachToDriver
	//! \brief		Attaches to the installed kernel virtual joystick driver and returns a handle
  //!           that may be used to feed or detach from the driver
  //-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API 
HANDLE AttachToDriver( void );

  //-------------------------------------------------------------------------------------------
	//	DetachFromDriver
	//! \brief		Stops any updates and fully detaches from the kernel virtual joystick driver
  //-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API 
BOOL DetachFromDriver( HANDLE driver );

  //-------------------------------------------------------------------------------------------
	//	BeginDriverUpdateLoop
	//! \brief		Begins feeding the virtual driver with the configured physical device mapping
  //-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API 
BOOL BeginDriverUpdateLoop( HANDLE driver );

  //-------------------------------------------------------------------------------------------
	//	EndDriverUpdateLoop
	//! \brief		Stops feeding the virtual driver
  //-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API 
BOOL EndDriverUpdateLoop( HANDLE driver );



//-------------------------------------------------------------------------------------------
	//	
	//! \brief		
  //-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API 
BOOL EnumerateDevices( HANDLE driver, DeviceEnumCB callbackFunct );

  //-------------------------------------------------------------------------------------------
	//	
	//! \brief		
  //-------------------------------------------------------------------------------------------
extern "C"
VJOYDRIVERINTERFACE_API 
BOOL SetDeviceIDs( HANDLE driver, const char *joystickGUID, const char *rudderGUID );
