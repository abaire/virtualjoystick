#pragma once

#include <dinput.h>
#include <vector>

#include "VJoyDriverInterface.h"

//! Sanity limit on the maximum number of attempts to acquire the physical device
#define MAX_ACQUIRE_RETRIES 512  

#define AXIS_MIN  -32767
#define AXIS_MAX  32767

//! Keycode to set if too many virtual keys are pressed at once.
#define HID_KEYBOARD_ERROR_ROLL_OVER ((UINT8)0x01)

// DirectInput reports POV values between 0 and 31500 (and -1 as a NULL val)
// However the driver expects a value between 0 and 7 (or -1 for NULL)
// It'd be nice to be able to use a full gradient, but DInput doesn't seem
//  to support any values > 31500, so it adds no value.
#define MAP_RANGE( val )  (INT8)(val > 0 ? val / 4500 : val)

#define SETBUTTON( offset, val ) SetButton(packet, offset, val)

#define ADDKEYDOWN( keycode ) AddKeyDown(packet, keycode)


//! \class  CJoystickDevice
//! \brief  Wraps a physical input device and maps its state into the kernel virtual joystick driver.
class CJoystickDevice
{
public:
    typedef std::vector<DeviceMapping> DeviceMappingVector;

    CJoystickDevice(LPDIRECTINPUT8, const DIDEVICEINSTANCE& device, const DeviceMappingVector& deviceMapping);

    BOOL Acquire();

    inline void Release(void)
    {
        if (m_inputDevice) m_inputDevice->Release();
        m_inputDevice = NULL;
    }

    inline BOOL SetEventNotification(HANDLE h)
    {
        if (m_inputDevice) {
            auto ret = m_inputDevice->SetEventNotification(h);
            return ret == DI_OK;
        }
        return FALSE;
    }

    //! \brief Polls the current physical device states and formats a HID DEVICE_PACKET message
    //!        that will be sent to the kernel miniport driver
    //!
    //! \warning  It is the responsibility of the caller to prevent any calls to Add/ClearMapping
    //!           while this function is being invoked!  Failure to do so may cause crashes 
    //!           or inconsistent state!
    inline BOOL GetVirtualStateUpdatePacket(VENDOR_DEVICE_PACKET& packet);


    inline BOOL IsPolled() const { return m_isPolled;  }

protected:

    BOOL EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE*);

    static BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* obj, VOID* context)
    {
        return ((CJoystickDevice*)context)->EnumObjectsCallback(obj);
    }

    //! \brief Polls for the current state of the physical device wrapped by this instance.
    inline BOOL PollPhysicalStick(void)
    {
        HRESULT hr = m_inputDevice->Poll();
        if (FAILED(hr))
        {
            UINT numRetries = 0;

            hr = m_inputDevice->Acquire();
            while (hr == DIERR_INPUTLOST && numRetries++ < MAX_ACQUIRE_RETRIES)
                hr = m_inputDevice->Acquire();
            return FALSE;
        }

        hr = m_inputDevice->GetDeviceState(sizeof(m_state), &m_state);
        if (FAILED(hr))
        {
            //printf( "ERROR checking dev state for joystick\n" );
            return FALSE;
        }

        return TRUE;
    }


protected:
    DIDEVICEINSTANCE m_deviceInstance; //!< Information about the physical device being wrapped by this instance
    LPDIRECTINPUTDEVICE8 m_inputDevice; //!< dinput handle to the physical device being wrapped by this instance
    BOOL m_isPolled;  //!< TRUE if the device must be polled rather than interrupt driven.

    DIJOYSTATE2 m_state; //!< The current state of the physical joystick represented by this instance

    static INT_PTR JOYSTATEAXISOFFSETS[8];
    //!< byte offset from the head of a DIJOYSTATE2 struct to the DWORD axis values

    //! Vector of mapping structure instance defining how to map the physical device state to the virtual multiplexed device
    DeviceMappingVector m_deviceMapping;
};


static __inline void SetButton(VENDOR_DEVICE_PACKET& packet, UINT32 index, BOOL val)
{
    UINT32 offset = index >> 3;
    UINT32 bit = 1 << (index & 0x07);

    if (val)
    {
        packet.joystick.Button[offset] |= bit;
    }
    else
    {
        packet.joystick.Button[offset] &= ~bit;
    }
}

static __inline void AddKeyDown(VENDOR_DEVICE_PACKET& packet, UINT32 keycode)
{
    UINT32 index = 0;
    UINT8 keycodeByte = static_cast<UINT8>(keycode);

    for (; index < sizeof(packet.keyboard.keycodes); ++index)
    {
        if (packet.keyboard.keycodes[index] == keycodeByte)
        {
            return;
        }

        if (!packet.keyboard.keycodes[index])
        {
            packet.keyboard.keycodes[index] = keycodeByte;
            return;
        }
    }

    memset(packet.keyboard.keycodes, HID_KEYBOARD_ERROR_ROLL_OVER, sizeof(packet.keyboard.keycodes));
}


inline void SetReportAxis(VENDOR_DEVICE_PACKET& report, AxisIndex axis, INT16 value)
{
    switch (axis)
    {
    case AxisIndex::axis_x:
        report.joystick.X = value;
        return;

    case AxisIndex::axis_y:
        report.joystick.Y = value;
        return;

    case AxisIndex::axis_throttle:
        report.joystick.Throttle = value;
        return;

    case AxisIndex::axis_rx:
        report.joystick.rX = value;
        return;

    case AxisIndex::axis_ry:
        report.joystick.rY = value;
        return;

    case AxisIndex::axis_rz:
        report.joystick.rZ = value;
        return;

    case AxisIndex::axis_slider:
        report.joystick.Slider = value;
        return;

    case AxisIndex::axis_dial:
        report.joystick.Dial = value;
        return;

    case AxisIndex::axis_rudder:
        report.joystick.Rudder = value;
        return;
    }
}

inline BOOL CJoystickDevice::GetVirtualStateUpdatePacket(VENDOR_DEVICE_PACKET& packet)
{
    if (!PollPhysicalStick())
        return FALSE;

    // NOTE: There is a concurrency issue here that we're requiring the caller to handle. It should
    // not be permissible for the device mapping vector to be modified (via Add/Clear mapping) while
    // we're fetching data.
    // The fetch must be a fast operation, however, and we don't want to have to lock a sem per device per poll
    DeviceMappingVector::iterator it = m_deviceMapping.begin();
    DeviceMappingVector::iterator itEnd = m_deviceMapping.end();
    for (; it != itEnd; ++it)
    {
        switch (it->destType)
        {
        case MappingType::mt_axis:
            {
                // JOYSTATEAXISOFFSETS holds the byte offset from the start of the joystate struct
                // to the various axis IDs.
                // DIJOYSTATE2 axes are held in 32 bit values, so we get a pointer to the proper address, cast
                // to UINT32 *, dereference as a UINT32, and then cast that value to an INT16 for return (we've
                // previously set the device up to return values in the 16 bit range)
                INT16 src = (INT16)*((LONG*)(((BYTE*)&m_state) + JOYSTATEAXISOFFSETS[it->srcIndex]));
                if (it->invert)
                {
                    src *= -1;
                }
                SetReportAxis(packet, static_cast<AxisIndex>(it->destIndex), src);
            }
            break;

        case MappingType::mt_pov:
            packet.joystick.POV = MAP_RANGE(m_state.rgdwPOV[it->srcIndex]);
            break;

        case MappingType::mt_button:
            {
                switch (it->srcType)
                {
                case MappingType::mt_button:
                    SETBUTTON(it->destIndex, (m_state.rgbButtons[it->srcIndex] == 0x80));
                    break;

                case MappingType::mt_pov:
                    {
                        BOOL buttonSet[4] = {FALSE,FALSE,FALSE,FALSE};

                        INT8 hatState = MAP_RANGE(m_state.rgdwPOV[it->srcIndex]);
                        switch (hatState)
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

                        SETBUTTON(it->destIndex, buttonSet[0]);
                        SETBUTTON(it->destIndex + 1, buttonSet[1]);
                        SETBUTTON(it->destIndex + 2, buttonSet[2]);
                        SETBUTTON(it->destIndex + 3, buttonSet[3]);
                    }
                    break;
                }
            }

        case MappingType::mt_key:
            ADDKEYDOWN(it->destIndex);
            break;
        }
    }

    return TRUE;
}
