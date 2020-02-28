//= I N C L U D E S ===========================================================================
#include "stdafx.h"
#include "DriverInterface.h"
#include <stdio.h>
#include <string.h>


//= D E F I N E S =============================================================================
static const GUID PRODUCT_VJOY = {0x0001EBA0, 0x0000, 0x0000, {0x00, 0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44}};

#define DEFAULT_LOOP_DELAY  15

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

#define AXIS_MIN  -32767
#define AXIS_MAX  32767

// Maximum number of failed consecutive HidD_SetFeature calls before we bomb out
#define MAX_CONSECUTIVE_HID_COMMUNICATION_FAILURES  5

// TEMP
// Define in order to preset the hat switch states as sets of 8 individual buttons
#define HATS_AS_BUTTONS
#define HAT_1_AS_BUTTONS_OFFSET   9     // First hat button offset


//= G L O B A L = V A R S =====================================================================
LPDIRECTINPUT8 CDriverInterface::m_pDI = NULL;

//= P R O T O T Y P E S =======================================================================
static BOOL CALLBACK EnumJoysticksForFrontend(const DIDEVICEINSTANCE* inst, VOID* pContext);


//= F U N C T I O N S =========================================================================

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
CDriverInterface::CDriverInterface(void) :
    m_deviceName()
    , m_driverHandle(INVALID_HANDLE_VALUE)
    , m_updateThreadHandle(INVALID_HANDLE_VALUE)
    , m_updateThreadRunning(FALSE)
    , m_updateLoopDelay(DEFAULT_LOOP_DELAY)
    , m_inputDeviceVector()
{
}

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
CDriverInterface::CDriverInterface(const std::string& deviceName) :
    m_deviceName(deviceName)
    , m_driverHandle(INVALID_HANDLE_VALUE)
    , m_updateThreadHandle(INVALID_HANDLE_VALUE)
    , m_updateThreadRunning(FALSE)
    , m_updateLoopDelay(DEFAULT_LOOP_DELAY)
    , m_inputDeviceVector()
{
    m_driverHandle = CreateFile(deviceName.c_str(),
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);
}


//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
BOOL CDriverInterface::RunUpdateThread(void)
{
    if (m_driverHandle == INVALID_HANDLE_VALUE ||
        m_updateThreadHandle != INVALID_HANDLE_VALUE)
        return FALSE;

    m_updateThreadRunning = TRUE;
    m_updateThreadHandle = CreateThread(NULL, 0, UpdateThreadProc, this, 0, NULL);

    return m_updateThreadHandle != INVALID_HANDLE_VALUE;
}

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
BOOL CDriverInterface::ExitUpdateThread(void)
{
    if (m_driverHandle == INVALID_HANDLE_VALUE)
        return FALSE;


    // Stop the thread if necessary
    if (m_updateThreadHandle != INVALID_HANDLE_VALUE)
    {
        m_updateThreadRunning = FALSE;
        WaitForSingleObject(m_updateThreadHandle, INFINITE);
        CloseHandle(m_updateThreadHandle);

        m_updateThreadHandle = INVALID_HANDLE_VALUE;
    }

    return TRUE;
}

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
void CDriverInterface::CloseDriverHandle(void)
{
    if (m_driverHandle == INVALID_HANDLE_VALUE)
        return;

    CloseHandle(m_driverHandle);
    m_driverHandle = INVALID_HANDLE_VALUE;
}


//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
BOOL CDriverInterface::OpenDirectXHandle(void)
{
    // Pop open dinput and grab our joysticks
    if (FAILED(DirectInput8Create(GetModuleHandle(NULL),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8A,
        (LPVOID*)&m_pDI,
        NULL)))
        return FALSE;


    return TRUE;
}


//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
DWORD CDriverInterface::UpdateThreadProc(void)
{
    if (!m_pDI)
        OpenDirectXHandle();

    // Get our list of input devices
    if (FAILED(m_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCB, this, DIEDFL_ATTACHEDONLY)))
    {
        SAFE_RELEASE(m_pDI);
        return 0;
    }


    DeviceVector::iterator it = m_inputDeviceVector.begin();
    DeviceVector::iterator itEnd = m_inputDeviceVector.end();
    for (; it != itEnd; ++it)
    {
        // Check to see if the device is mapped

        // Acquire the stick
        if (!it->Acquire())
        {
            SAFE_RELEASE(m_pDI);
            return 0;
        }
    }


    // Track the number of consecutive failed setHID calls
    UINT32 numFailures = 0;

    // Device packet to send via HidD_SetFeature
    DEVICE_PACKET packet;
    memset(&packet, 0, sizeof(packet));
    packet.id = REPORTID_VENDOR;

    while (m_updateThreadRunning)
    {
        // Invalidate the POV axes
        packet.report.POV[0] =
            packet.report.POV[1] =
            packet.report.POV[2] =
            packet.report.POV[3] = -1;

        // Fetch data from our devices into the report packet
        it = m_inputDeviceVector.begin();
        itEnd = m_inputDeviceVector.end();
        for (; it != itEnd; ++it)
            it->GetVirtualStateUpdatePacket(packet);

        /*
          #define DUMP_DEVICE_REPORT( _v_ ) \
            { char buffer[1024] = {0}; \
              sprintf( buffer,  \
                        "\tX: 0x%X\n" \
                          "\tY: 0x%X\n" \
                          "\tZ: 0x%X\n" \
                          "\trX: 0x%X\n" \
                          "\trY: 0x%X\n" \
                          "\trZ: 0x%X\n" \
                          "\tDial[0]: 0x%X\n" \
                          "\tDial[1]: 0x%X\n" \
                          "\tPOV[0]: 0x%X\n" \
                          "\tPOV[1]: 0x%X\n" \
                          "\tPOV[2]: 0x%X\n" \
                          "\tPOV[3]: 0x%X\n" \
                          "\tButton[0]: 0x%X\n" \
                          "\tButton[1]: 0x%X\n" \
                          "\tButton[2]: 0x%X\n" \
                          "\tButton[3]: 0x%X\n" \
                          "\tButton[4]: 0x%X\n" \
                          "\tButton[5]: 0x%X\n" \
                          , ((PDEVICE_REPORT)(_v_))->X \
                          , ((PDEVICE_REPORT)(_v_))->Y \
                          , ((PDEVICE_REPORT)(_v_))->Z \
                          , ((PDEVICE_REPORT)(_v_))->rX \
                          , ((PDEVICE_REPORT)(_v_))->rY \
                          , ((PDEVICE_REPORT)(_v_))->rZ \
                          , ((PDEVICE_REPORT)(_v_))->Slider[0] \
                          , ((PDEVICE_REPORT)(_v_))->Slider[1] \
                          , ((PDEVICE_REPORT)(_v_))->POV[0] \
                          , ((PDEVICE_REPORT)(_v_))->POV[1] \
                          , ((PDEVICE_REPORT)(_v_))->POV[2] \
                          , ((PDEVICE_REPORT)(_v_))->POV[3] \
                          , ((PDEVICE_REPORT)(_v_))->Button[0] \
                          , ((PDEVICE_REPORT)(_v_))->Button[1] \
                          , ((PDEVICE_REPORT)(_v_))->Button[2] \
                          , ((PDEVICE_REPORT)(_v_))->Button[3] \
                          , ((PDEVICE_REPORT)(_v_))->Button[4] \
                          , ((PDEVICE_REPORT)(_v_))->Button[5] ); \
                  OutputDebugString( buffer ); \
            }

            DUMP_DEVICE_REPORT( &packet.report );
        */

        // Send the request on to the driver
        DWORD bytesWritten;
        if (!WriteFile(m_driverHandle,
                       &packet,
                       sizeof(packet),
                       &bytesWritten,
                       NULL))
        {
            TCHAR buffer[1024];
            sprintf_s(buffer,
                      1024,
                      "Failed to write output report to the driver: failure #%lu, error %d\n",
                      numFailures,
                      GetLastError());
            OutputDebugString(buffer);

            if (++numFailures > MAX_CONSECUTIVE_HID_COMMUNICATION_FAILURES)
                break;
        }
        else
            numFailures = 0;


        Sleep(m_updateLoopDelay);
    }

    // Kill DirectInput 
    it = m_inputDeviceVector.begin();
    itEnd = m_inputDeviceVector.end();
    for (; it != itEnd; ++it)
        it->Release();

    m_inputDeviceVector.clear();

    SAFE_RELEASE(m_pDI);

    return 0;
}


//-------------------------------------------------------------------------------------------
//	EnumerateDevices
//-------------------------------------------------------------------------------------------
BOOL CDriverInterface::EnumerateDevices(DeviceEnumCB cb)
{
    BOOL releaseAfterOp = FALSE;
    if (!m_pDI)
    {
        OpenDirectXHandle();
        releaseAfterOp = TRUE;
    }

    if (FAILED(m_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksForFrontend, cb, DIEDFL_ATTACHEDONLY)))
        return FALSE;

    if (releaseAfterOp)
    SAFE_RELEASE(m_pDI);

    return TRUE;
}

//-------------------------------------------------------------------------------------------
//	
//-------------------------------------------------------------------------------------------
static BOOL CALLBACK EnumJoysticksForFrontend(const DIDEVICEINSTANCE* inst, VOID* pContext)
{
    CDriverInterface::DeviceEnumCB cb = CDriverInterface::DeviceEnumCB(pContext);

    // Skip our own virtual joystick
    if (inst->guidProduct != PRODUCT_VJOY)
    {
        char guid[256] = {0};
        _snprintf_s(guid,
                    255,
                    255,
                    "%8.8X%4.4X%4.4X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                    inst->guidInstance.Data1,
                    inst->guidInstance.Data2,
                    inst->guidInstance.Data3,
                    inst->guidInstance.Data4[0],
                    inst->guidInstance.Data4[1],
                    inst->guidInstance.Data4[2],
                    inst->guidInstance.Data4[3],
                    inst->guidInstance.Data4[4],
                    inst->guidInstance.Data4[5],
                    inst->guidInstance.Data4[6],
                    inst->guidInstance.Data4[7]);

        cb(inst->tszInstanceName, guid);
    }

    return DIENUM_CONTINUE;
}


//-------------------------------------------------------------------------------------------
//	EnumJoysticksCB
//-------------------------------------------------------------------------------------------
BOOL CDriverInterface::EnumJoysticksCB(const DIDEVICEINSTANCE* inst)
{
#define MAP( db, di, sb, si )  m.destBlock = db, m.destIndex = di, m.srcBlock = sb, m.srcIndex = si, m.invert = FALSE
#define MAPAXIS( di, si ) m.destBlock = m.srcBlock = CJoystickDevice::DS_AXIS, m.destIndex = di, m.srcIndex = si, m.invert = FALSE
#define MAPBUTTON( index )  m.destBlock = m.srcBlock = CJoystickDevice::DS_BUTTON, m.destIndex = m.srcIndex = index, m.invert = FALSE
    CJoystickDevice::DeviceMapping m;

    if (inst->guidInstance == m_joystickGUID)
    {
        CJoystickDevice dev(m_pDI, *inst);

        MAPAXIS(CJoystickDevice::AXIS_X, CJoystickDevice::AXIS_X);
        dev.AddMapping(m);

        MAPAXIS(CJoystickDevice::AXIS_Y, CJoystickDevice::AXIS_Y);
        dev.AddMapping(m);

        MAPAXIS(CJoystickDevice::AXIS_Z, CJoystickDevice::AXIS_S0);
        dev.AddMapping(m);

        MAPAXIS(CJoystickDevice::AXIS_S0, CJoystickDevice::AXIS_S1);
        dev.AddMapping(m);

        MAPAXIS(CJoystickDevice::AXIS_S1, CJoystickDevice::AXIS_RZ);
        dev.AddMapping(m);

        // Map buttons
        for (UINT32 i = 0; i < 64; ++i)
        {
            MAPBUTTON(i);
            dev.AddMapping(m);
        }

        // Map our POV's
#ifdef HATS_AS_BUTTONS
        // Clear the old button mappings
        MAPBUTTON(HAT_1_AS_BUTTONS_OFFSET);
        dev.ClearMapping(m);
        MAPBUTTON(HAT_1_AS_BUTTONS_OFFSET + 1);
        dev.ClearMapping(m);
        MAPBUTTON(HAT_1_AS_BUTTONS_OFFSET + 2);
        dev.ClearMapping(m);
        MAPBUTTON(HAT_1_AS_BUTTONS_OFFSET + 3);
        dev.ClearMapping(m);

        MAP(CJoystickDevice::DS_BUTTON, HAT_1_AS_BUTTONS_OFFSET, CJoystickDevice::DS_POV, 0);
        dev.AddMapping(m);
#else
        MAP(CJoystickDevice::DS_POV, 0, CJoystickDevice::DS_POV, 0);
        dev.AddMapping(m);
#endif

        m_inputDeviceVector.push_back(dev);
    }
    else if (inst->guidInstance == m_rudderGUID)
    {
        CJoystickDevice dev(m_pDI, *inst);

        MAPAXIS(CJoystickDevice::AXIS_RX, CJoystickDevice::AXIS_X); // X axis is left pedal (inverted)
        dev.AddMapping(m);

        MAPAXIS(CJoystickDevice::AXIS_RY, CJoystickDevice::AXIS_Y); // Y axis is right pedal (inverted)
        dev.AddMapping(m);

        MAPAXIS(CJoystickDevice::AXIS_RZ, CJoystickDevice::AXIS_RZ); // RZ axis is rudder
        dev.AddMapping(m);

        m_inputDeviceVector.push_back(dev);
    }

    return DIENUM_CONTINUE;
}
