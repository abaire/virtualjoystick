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

// Special ID used to indicate that a mapping should be ignored.
#define UNMAPPED_INDEX 0xFFFF

extern "C" {

//! \enum   MappingType
//! \brief  Defines the type of value to be mapped to the virtual driver
typedef enum _MappingType
{
    mt_axis = 0,
    mt_pov,
    mt_button
} MappingType;

//! \enum   AxisIndex
//! \brief  Defines the index of the various axes in the JOYSTATEAXISOFFSETS
typedef enum _AxisIndex
{
    axis_none = UNMAPPED_INDEX,
    axis_x = 0,
    axis_y,
    axis_throttle,
    axis_rx,
    axis_ry,
    axis_rz,
    axis_slider,
    axis_dial,

    // Additional axes are not available in DIJOYSTATE2.
    axis_rudder,
} AxisIndex;

//! \struct DeviceMapping
//! \brief  Maps a specific dinput state variable to an output slot in the virtual joystick
typedef struct _DeviceMapping
{
    MappingType destBlock; //!< The type of the target virtual joystick state
    DWORD destIndex; //!< The index of the target virtual joystick state

    MappingType srcBlock; //!< The type of the source state
    DWORD srcIndex; //!< The index of the source joystick state

    BOOL invert; //!< Whether or not we should logically invert the physical state when injecting the virtual device
} DeviceMapping;

typedef void (CALLBACK* DeviceEnumCB)(const char* name, const char* guid);
typedef void (CALLBACK* DeviceInfoCB)(MappingType type, const char* name, UINT32 index);


//= P R O T O T Y P E S =======================================================================

// Attaches to the installed kernel virtual joystick driver and returns a handle
// that may be used to manipulate the driver.
VJOYDRIVERINTERFACE_API
HANDLE AttachToVirtualJoystickDriver(void);

// Stops any updates and fully detaches from the kernel virtual joystick driver
VJOYDRIVERINTERFACE_API
BOOL DetachFromVirtualJoystickDriver(HANDLE driver);

// Begins feeding the virtual driver with the configured physical device mapping
VJOYDRIVERINTERFACE_API
BOOL BeginDriverUpdateLoop(HANDLE driver);

// Stops feeding the virtual driver
VJOYDRIVERINTERFACE_API
BOOL EndDriverUpdateLoop(HANDLE driver);

// callbackFunc will be invoked synchronously multiple times for each device.
// Enumerates physical devices appropriate for mapping to the virtual driver.
VJOYDRIVERINTERFACE_API
BOOL EnumerateDevices(HANDLE driver, DeviceEnumCB callbackFunc);

// callbackFunc will be invoked synchronously multiple times for each object in the device.
VJOYDRIVERINTERFACE_API
BOOL GetDeviceInfo(HANDLE driver, _In_ const char* deviceGUID, DeviceInfoCB callbackFunc);

VJOYDRIVERINTERFACE_API
BOOL SetDeviceMapping(HANDLE driver, const char* deviceGUID, const DeviceMapping* mappings, size_t mappingCount);

VJOYDRIVERINTERFACE_API
BOOL ClearDeviceMappings(HANDLE driver);

} // extern "C"
