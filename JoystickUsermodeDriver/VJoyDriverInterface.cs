using System;
using System.Collections.Generic;
using System.Data;
using System.Runtime.InteropServices;
using System.Text;

namespace JoystickUsermodeDriver
{
    class VJoyDriverInterface
    {
        // Keep in sync with VJoyDriverInterface.h

        public enum MappingType
        {
            DS_AXIS = 0,
            DS_POV,
            DS_BUTTON
        }

        public enum AxisIndex
        {
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
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct DeviceMapping
        {
            MappingType destBlock; //!< The type of the target virtual joystick state
            UInt32 destIndex; //!< The index of the target virtual joystick state

            MappingType srcBlock; //!< The type of the source state
            UInt32 srcIndex; //!< The index of the source joystick state

            bool invert;

            public DeviceMapping(
                MappingType destBlock,
                UInt32 destIndex,
                MappingType srcBlock,
                UInt32 srcIndex,
                bool invert)
            {
                this.destBlock = destBlock;
                this.destIndex = destIndex;
                this.srcBlock = srcBlock;
                this.srcIndex = srcIndex;
                this.invert = invert;
            }
        }

        public static UInt32 INVALID_HANDLE_VALUE = (UInt32) 0xFFFFFFFF;

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "AttachToVirtualJoystickDriver",
            SetLastError = false)]
        public static extern UInt32 AttachToVirtualJoystickDriver();

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "BeginDriverUpdateLoop",
            SetLastError = false)]
        public static extern bool BeginDriverUpdateLoop(UInt32 h);

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "EndDriverUpdateLoop",
            SetLastError = false)]
        public static extern bool EndDriverUpdateLoop(UInt32 h);

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll",
            EntryPoint = "DetachFromVirtualJoystickDriver", SetLastError = false)]
        public static extern bool DetachFromVirtualJoystickDriver(UInt32 h);

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "SetDeviceMapping",
            SetLastError = false)]
        public static extern bool SetDeviceMapping(UInt32 h,
            [MarshalAs(UnmanagedType.LPStr)] string deviceGUID,
            DeviceMapping[] mappings, 
            UInt32 mappingCount);

        public delegate void DeviceEnumCallback([MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string guid);

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "EnumerateDevices",
            SetLastError = false)]
        public static extern bool EnumerateDevices(UInt32 h, DeviceEnumCallback cb);
    }
}