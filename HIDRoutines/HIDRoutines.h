/*!
 * File:      HIDRoutines.h
 * Created:   2006/10/07 17:18
 * \file      vjoy\VJoyDriverInterface\HIDRoutines.h
 * \brief
 */

#ifndef _HIDROUTINES_H__
#define _HIDROUTINES_H__

 //= I N C L U D E S ===========================================================================
#include <string>

//= P R O T O T Y P E S =======================================================================

  //-------------------------------------------------------------------------------------------
    //	FindJoystickDeviceName
    //! \brief		Finds the unique device name of the running virtual joystick driver instance
  //!
  //! \param    ret - [OUT] - String pointer to receive a TCHAR buffer containing the driver name
  //!
  //! \return   BOOL - TRUE if the device was successfully found and attached, otherwise FALSE
  //-------------------------------------------------------------------------------------------
BOOL FindJoystickDeviceName(TCHAR** ret);


#endif
