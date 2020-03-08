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

// Keyboard key modifiers.
#define MODIFIER_LEFT_CTRL (1 << 0)
#define MODIFIER_LEFT_SHIFT (1 << 1)
#define MODIFIER_LEFT_ALT (1 << 2)
#define MODIFIER_LEFT_GUI (1 << 3)
#define MODIFIER_RIGHT_CTRL (1 << 4)
#define MODIFIER_RIGHT_SHIFT (1 << 5)
#define MODIFIER_RIGHT_ALT (1 << 6)
#define MODIFIER_RIGHT_GUI (1 << 7)

#define KEYCODE_F1 0x013A
#define KEYCODE_F2 0x013B
#define KEYCODE_F3 0x013C
#define KEYCODE_F4 0x013D
#define KEYCODE_F5 0x013E
#define KEYCODE_F6 0x013F
#define KEYCODE_F7 0x0140
#define KEYCODE_F8 0x0141
#define KEYCODE_F9 0x0142
#define KEYCODE_F10 0x0143
#define KEYCODE_F11 0x0144
#define KEYCODE_F12 0x0145

#define KEYCODE_RIGHT_ARROW 0x014F
#define KEYCODE_LEFT_ARROW 0x0150
#define KEYCODE_DOWN_ARROW 0x0151
#define KEYCODE_UP_ARROW 0x0152

#define KEYCODE_KEYPAD_1 0x0159
#define KEYCODE_KEYPAD_2 0x015A
#define KEYCODE_KEYPAD_3 0x015B
#define KEYCODE_KEYPAD_4 0x015C
#define KEYCODE_KEYPAD_5 0x015D
#define KEYCODE_KEYPAD_6 0x015E
#define KEYCODE_KEYPAD_7 0x015F
#define KEYCODE_KEYPAD_8 0x0160
#define KEYCODE_KEYPAD_9 0x0161
#define KEYCODE_KEYPAD_0 0x0162

#define KEYCODE_LEFT_CTRL (MODIFIER_LEFT_CTRL << 16)
#define KEYCODE_LEFT_SHIFT (MODIFIER_LEFT_SHIFT << 16)
#define KEYCODE_LEFT_ALT (MODIFIER_LEFT_ALT << 16)
#define KEYCODE_LEFT_GUI (MODIFIER_LEFT_GUI << 16)
#define KEYCODE_RIGHT_CTRL (MODIFIER_RIGHT_CTRL << 16)
#define KEYCODE_RIGHT_SHIFT (MODIFIER_RIGHT_SHIFT << 16)
#define KEYCODE_RIGHT_ALT (MODIFIER_RIGHT_ALT << 16)
#define KEYCODE_RIGHT_GUI (MODIFIER_RIGHT_GUI << 16)

extern "C" {

//! \enum   MappingType
//! \brief  Defines the type of value to be mapped to the virtual driver
enum class MappingType : UINT32
{
    mt_axis = 0,
    mt_pov,
    mt_button,
    mt_key
};

//! \enum   AxisIndex
//! \brief  Defines the index of the various axes in the JOYSTATEAXISOFFSETS
enum class AxisIndex : UINT32
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
};

enum class Transform : BYTE
{
    transform_none = 0,
    transform_invert_axis,
    transform_rapid_fire,
    transform_edge_detect
};

#include <pshpack1.h>

//! \struct DeviceMapping
//! \brief  Maps a specific dinput state variable to an output slot in the virtual joystick
typedef struct _DeviceMapping
{
    MappingType destType; //!< The type of the target virtual joystick state
    UINT32 destIndex; //!< The index of the target virtual joystick state

    MappingType srcType; //!< The type of the source state
    UINT32 srcIndex; //!< The index of the source joystick state

    //! Axis->Axis: inverts the reported physical position during mapping.
    //! Button/POV->Key: fires a key event on physical state change rather than tracking physical state.
    //!                  This means an instantaneous button will 
    Transform transform;

    //! For rapid fire and edge detect: milliseconds to simulate "pressed" state.
    BYTE downMillis;

    //! For rapid fire: milliseconds between "pressed" events.
    BYTE repeatMillis;

    //! Sensitivity boost percentage. Values > 0 cause the virtual axis to reach its maximum/minimum
    //! value at (100 - sensitivityBoost)% of the physical range.
    BYTE sensitivityBoost;
} DeviceMapping;

//! \struct VirtualDeviceState
//! \brief Models the state of the virtual joystick.
typedef struct _VirtualDeviceState
{
    INT16 x;
    INT16 y;

    INT16 throttle;

    INT16 rX;
    INT16 rY;
    INT16 rZ;

    INT16 slider;
    INT16 dial;

    INT16 rudder;

    UINT8 _PAD_0[2];

    UINT8 povNorth;
    UINT8 povEast;
    UINT8 povSouth;
    UINT8 povWest;

    UINT8 button[16]; // 128 button bits

    UINT32 keycodes[7];

    UINT8 modifierKeys;
} VirtualDeviceState;

#include <poppack.h>

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

VJOYDRIVERINTERFACE_API
UINT32 UpdateLoopDelay(HANDLE driver);

VJOYDRIVERINTERFACE_API
BOOL SetUpdateLoopDelay(HANDLE driver, UINT32 delay);

// Forces the virtual joystick device to present the given state.
// If allowOverride is TRUE, any mapped explicit values from physical devices will override
// components in the virtual state.
//
// Passing NULL will clear the forced state.
//
// After 
VJOYDRIVERINTERFACE_API
BOOL SetVirtualDeviceState(HANDLE driver, const VirtualDeviceState* state, BOOL allowOverride);

} // extern "C"
