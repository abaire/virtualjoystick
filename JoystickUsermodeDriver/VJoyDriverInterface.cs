using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace JoystickUsermodeDriver
{
  class VJoyDriverInterface
  {
    public static UInt32 INVALID_HANDLE_VALUE = (UInt32)0xFFFFFFFF;

    [System.Runtime.InteropServices.DllImport( "VJoyDirectXBridge.dll", EntryPoint = "AttachToDriver", SetLastError = false )]
    public static extern UInt32 AttachToDriver();

    [System.Runtime.InteropServices.DllImport( "VJoyDirectXBridge.dll", EntryPoint = "BeginDriverUpdateLoop", SetLastError = false )]
    public static extern bool BeginDriverUpdateLoop( UInt32 h );

    [System.Runtime.InteropServices.DllImport( "VJoyDirectXBridge.dll", EntryPoint = "EndDriverUpdateLoop", SetLastError = false )]
    public static extern bool EndDriverUpdateLoop( UInt32 h );

    [System.Runtime.InteropServices.DllImport( "VJoyDirectXBridge.dll", EntryPoint = "DetachFromDriver", SetLastError = false )]
    public static extern bool DetachFromDriver( UInt32 h );
    
    [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "SetDeviceIDs", SetLastError = false)]
    public static extern bool SetDeviceIDs( UInt32 h, 
                                           [MarshalAs(UnmanagedType.LPStr)] string joystickGUID, 
                                           [MarshalAs(UnmanagedType.LPStr)] string thottleGUID );

    
    public delegate void DeviceEnumCallback( [MarshalAs(UnmanagedType.LPStr)] string name,
                                             [MarshalAs(UnmanagedType.LPStr)] string guid );

    [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "EnumerateDevices", SetLastError = false)]
    public static extern bool EnumerateDevices( UInt32 h, DeviceEnumCallback cb );

  }
}
