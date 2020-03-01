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
static BOOL CALLBACK EnumJoysticksForFrontend(const DIDEVICEINSTANCE* inst, VOID* context);

//= F U N C T I O N S =========================================================================

CDriverInterface::CDriverInterface(const std::string& deviceName) :
    m_deviceName(deviceName)
    , m_updateThreadHandle(INVALID_HANDLE_VALUE)
    , m_updateThreadRunning(FALSE)
    , m_updateLoopDelay(DEFAULT_LOOP_DELAY)
{
    m_driverHandle = CreateFile(deviceName.c_str(),
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);

    m_interruptEvent = CreateEvent(
        NULL, // default security attributes
        FALSE,
        FALSE,
        "deviceDataReady"
    );
}

CDriverInterface::CDriverInterface(CDriverInterface&& o) noexcept
{
    *this = std::move(o);
}

CDriverInterface::CDriverInterface() :
    m_driverHandle(INVALID_HANDLE_VALUE)
    , m_updateThreadHandle(INVALID_HANDLE_VALUE)
    , m_updateThreadRunning(FALSE)
    , m_updateLoopDelay(DEFAULT_LOOP_DELAY)
    , m_interruptEvent(INVALID_HANDLE_VALUE)
{
}

CDriverInterface::~CDriverInterface()
{
    CloseInterruptEvent();
    CloseDriverHandle();
}

CDriverInterface& CDriverInterface::operator=(CDriverInterface&& o)
{
    if (&o == this) { return *this; }

    ExitUpdateThread();
    CloseInterruptEvent();
    CloseDriverHandle();

    m_deviceName = std::move(o.m_deviceName);
    m_driverHandle = std::exchange(o.m_driverHandle, INVALID_HANDLE_VALUE);
    m_updateThreadHandle = std::exchange(o.m_updateThreadHandle, INVALID_HANDLE_VALUE);
    m_updateThreadRunning = std::move(o.m_updateThreadRunning);
    m_interruptEvent = std::exchange(o.m_interruptEvent, INVALID_HANDLE_VALUE);
    m_updateLoopDelay = std::move(o.m_updateLoopDelay);
    m_pDI = std::move(o.m_pDI);
    o.m_pDI = NULL;
    m_inputDeviceVector = std::move(o.m_inputDeviceVector);
    m_deviceGUIDMapping = std::move(o.m_deviceGUIDMapping);

    return *this;
}

BOOL CDriverInterface::RunUpdateThread(void)
{
    if (m_driverHandle == INVALID_HANDLE_VALUE ||
        m_updateThreadHandle != INVALID_HANDLE_VALUE)
        return FALSE;

    m_updateThreadRunning = TRUE;
    m_updateThreadHandle = CreateThread(NULL, 0, UpdateThreadProc, this, 0, NULL);

    return m_updateThreadHandle != INVALID_HANDLE_VALUE;
}

BOOL CDriverInterface::ExitUpdateThread(void)
{
    if (m_driverHandle == INVALID_HANDLE_VALUE)
        return FALSE;

    // Stop the thread if necessary
    if (m_updateThreadHandle != INVALID_HANDLE_VALUE)
    {
        m_updateThreadRunning = FALSE;
        SetEvent(m_interruptEvent);
        WaitForSingleObject(m_updateThreadHandle, INFINITE);
        CloseHandle(m_updateThreadHandle);
        m_updateThreadHandle = INVALID_HANDLE_VALUE;
    }

    return TRUE;
}

void CDriverInterface::CloseDriverHandle(void)
{
    if (m_driverHandle == INVALID_HANDLE_VALUE)
        return;

    CloseHandle(m_driverHandle);
    m_driverHandle = INVALID_HANDLE_VALUE;
}

void CDriverInterface::CloseInterruptEvent(void)
{
    if (m_interruptEvent == INVALID_HANDLE_VALUE)
        return;

    CloseHandle(m_interruptEvent);
    m_interruptEvent = INVALID_HANDLE_VALUE;
}

BOOL CDriverInterface::OpenDirectXHandle(void)
{
    if (FAILED(DirectInput8Create(GetModuleHandle(NULL),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8A,
        (LPVOID*)&m_pDI,
        NULL)))
        return FALSE;

    return TRUE;
}

BOOL CDriverInterface::AcquireDevices(void)
{
    DeviceVector::iterator it = m_inputDeviceVector.begin();
    DeviceVector::iterator itEnd = m_inputDeviceVector.end();
    for (; it != itEnd; ++it)
    {
        if (!it->Acquire())
        {
            return FALSE;
        }
    }
    return TRUE;
}

void CDriverInterface::ReleaseDevices()
{
    DeviceVector::iterator it = m_inputDeviceVector.begin();
    DeviceVector::iterator itEnd = m_inputDeviceVector.end();
    for (; it != itEnd; ++it)
    {
        it->Release();
    }
}

BOOL CDriverInterface::SetNotificationOnDevices(HANDLE eventHandle)
{
    DeviceVector::iterator it = m_inputDeviceVector.begin();
    DeviceVector::iterator itEnd = m_inputDeviceVector.end();
    for (; it != itEnd; ++it)
    {
        if (!it->SetEventNotification(eventHandle))
        {
            return FALSE;
        }
    }
    return TRUE;
}

DWORD CDriverInterface::UpdateThreadProc(void)
{
    if (!m_pDI)
        OpenDirectXHandle();

    if (FAILED(m_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCB, this, DIEDFL_ATTACHEDONLY)))
    {
        SAFE_RELEASE(m_pDI);
        return 0;
    }

    BOOL usePolling = FALSE;
    DeviceVector::iterator it = m_inputDeviceVector.begin();
    DeviceVector::iterator itEnd = m_inputDeviceVector.end();
    for (; it != itEnd; ++it)
    {
        if (it->IsPolled())
        {
            usePolling = TRUE;
        }
    }

#if 0
    if (usePolling)
    {
        RunPollingLoop();
    }
    else
    {
        RunInterruptLoop();
    }
#else
    RunPollingLoop();
#endif

    SAFE_RELEASE(m_pDI);

    return 0;
}

void CDriverInterface::RunPollingLoop(void)
{
    if (!AcquireDevices())
    {
        return;
    }

    UINT32 numFailures = 0;

    DEVICE_PACKET packet;
    memset(&packet, 0, sizeof(packet));
    packet.id = REPORTID_VENDOR;

    while (m_updateThreadRunning)
    {
        // Invalidate the POV axis
        packet.report.POV = -1;

        DeviceVector::iterator it = m_inputDeviceVector.begin();
        DeviceVector::iterator itEnd = m_inputDeviceVector.end();
        for (; it != itEnd; ++it)
            it->GetVirtualStateUpdatePacket(packet);

        DWORD bytesWritten;
        if (!WriteFile(m_driverHandle,
                       &packet,
                       sizeof(packet),
                       &bytesWritten,
                       NULL))
        {
#if _DEBUG
            TCHAR buffer[1024];
            sprintf_s(buffer,
                      1024,
                      "Failed to write output report to the driver: failure #%lu, error %d\n",
                      numFailures,
                      GetLastError());
            OutputDebugString(buffer);
#endif

            if (++numFailures > MAX_CONSECUTIVE_HID_COMMUNICATION_FAILURES)
                break;
        }
        else
            numFailures = 0;

        Sleep(m_updateLoopDelay);
    }

    ReleaseDevices();
}

#if 0
void CDriverInterface::RunInterruptLoop(void)
{
    UINT32 numFailures = 0;

    DEVICE_PACKET packet;
    memset(&packet, 0, sizeof(packet));
    packet.id = REPORTID_VENDOR;

    if (!SetNotificationOnDevices(m_interruptEvent))
    {
#if _DEBUG
        OutputDebugString("Failed to set interrupt event on devices.");
#endif
        return;
    }

    if (!AcquireDevices())
    {
#if _DEBUG
        OutputDebugString("Failed to acquire devices.");
#endif
        return;
    }

    while (m_updateThreadRunning)
    {
        auto result = WaitForSingleObject(m_interruptEvent, 5000);
        switch (result)
        {
        case WAIT_OBJECT_0:
            break;

        case WAIT_ABANDONED:
        case WAIT_TIMEOUT:
            continue;

        case WAIT_FAILED:
            {
#if _DEBUG
                TCHAR buffer[1024];
                sprintf_s(buffer,
                          1024,
                          "Failed to wait on interrupt event:error %d\n",
                          GetLastError());
                OutputDebugString(buffer);
#endif
            }
            continue;
        }

        // Invalidate the POV axis
        packet.report.POV = -1;

        // Fetch data from our devices into the report packet
        DeviceVector::iterator it = m_inputDeviceVector.begin();
        DeviceVector::iterator itEnd = m_inputDeviceVector.end();
        for (; it != itEnd; ++it)
            it->GetVirtualStateUpdatePacket(packet);

        // Send the request on to the driver
        DWORD bytesWritten;
        if (!WriteFile(m_driverHandle,
                       &packet,
                       sizeof(packet),
                       &bytesWritten,
                       NULL))
        {
#if _DEBUG
            TCHAR buffer[1024];
            sprintf_s(buffer,
                      1024,
                      "Failed to write output report to the driver: failure #%lu, error %d\n",
                      numFailures,
                      GetLastError());
            OutputDebugString(buffer);
#endif

            if (++numFailures > MAX_CONSECUTIVE_HID_COMMUNICATION_FAILURES)
                break;
        }
        else
            numFailures = 0;
    }

    ReleaseDevices();
    SetNotificationOnDevices(NULL);
}
#endif

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
    {
        SAFE_RELEASE(m_pDI);
    }

    return TRUE;
}


BOOL CDriverInterface::GetDeviceInfo(
    const GUID& deviceGUID,
    DeviceObjectInfoVector& axes,
    DeviceObjectInfoVector& buttons,
    DeviceObjectInfoVector& povs)
{
    axes.clear();
    buttons.clear();
    povs.clear();

    BOOL releaseAfterOp = FALSE;
    BOOL ret = TRUE;
    if (!m_pDI)
    {
        OpenDirectXHandle();
        releaseAfterOp = TRUE;
    }

    LPDIRECTINPUTDEVICE8 device;
    if (FAILED(m_pDI->CreateDevice(deviceGUID, &device, NULL)))
    {
        ret = FALSE;
        goto cleanup;
    }

    DIDEVCAPS capabilities;
    capabilities.dwSize = sizeof(DIDEVCAPS);
    if (FAILED(device->GetCapabilities(&capabilities)))
    {
        ret = FALSE;
        goto cleanup;
    }

    auto numAxes = capabilities.dwAxes;
    auto numButtons = capabilities.dwButtons;
    auto numPOVs = capabilities.dwPOVs;

    auto callback = [](const DIDEVICEOBJECTINSTANCE* inst, VOID* pContext)
    {
        DeviceObjectInfoVector* info = static_cast<DeviceObjectInfoVector*>(pContext);
        UINT32 instance = DIDFT_GETINSTANCE(inst->dwType);
        info->push_back(DeviceObjectInfo(inst->tszName, instance));
        return DIENUM_CONTINUE;
    };

    if (numAxes && FAILED(device->EnumObjects(callback, &axes, DIDFT_AXIS)))
    {
        ret = FALSE;
        goto cleanup;
    }

    if (numButtons && FAILED(device->EnumObjects(callback, &buttons, DIDFT_BUTTON)))
    {
        ret = FALSE;
        goto cleanup;
    }

    if (numPOVs && FAILED(device->EnumObjects(callback, &povs, DIDFT_POV)))
    {
        ret = FALSE;
        goto cleanup;
    }

cleanup:
    if (releaseAfterOp)
    {
        SAFE_RELEASE(m_pDI);
    }

    return ret;
}

static BOOL CALLBACK EnumJoysticksForFrontend(const DIDEVICEINSTANCE* inst, VOID* pContext)
{
    CDriverInterface::DeviceEnumCB cb = static_cast<CDriverInterface::DeviceEnumCB>(pContext);

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


BOOL CDriverInterface::EnumJoysticksCB(const DIDEVICEINSTANCE* inst)
{
    DeviceIDMapping::iterator it = m_deviceGUIDMapping.find(inst->guidInstance);
    if (it == m_deviceGUIDMapping.end())
    {
        return DIENUM_CONTINUE;
    }

    CJoystickDevice dev(m_pDI, *inst, it->second);
    m_inputDeviceVector.push_back(dev);

    return DIENUM_CONTINUE;
}
