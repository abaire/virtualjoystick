using Microsoft.Win32;
using System;
using System.Runtime.InteropServices;

namespace JoystickUsermodeDriver
{
    class VJoyDriverInterface
    {
        // Keep in sync with VJoyDriverInterface.h

        public enum MappingType
        {
            axis = 0,
            pov,
            button
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

            Int32 invert;

            private const string ValueDestBlock = "destType";
            private const string ValueDestIndex = "destIndex";
            private const string ValueSrcBlock = "srcType";
            private const string ValueSrcIndex = "srcIndex";
            private const string ValueInvert = "invert";

            public DeviceMapping(
                MappingType destBlock,
                UInt32 destIndex,
                MappingType srcBlock,
                UInt32 srcIndex,
                bool invert = false)
            {
                this.destBlock = destBlock;
                this.destIndex = destIndex;
                this.srcBlock = srcBlock;
                this.srcIndex = srcIndex;
                this.invert = invert ? 1 : 0;
            }

            public DeviceMapping(
                MappingType destBlock,
                AxisIndex destIndex,
                MappingType srcBlock,
                AxisIndex srcIndex,
                bool invert = false)
                : this(destBlock, (UInt32)destIndex, srcBlock, (UInt32)srcIndex, invert)
            {
            }

            public DeviceMapping(
                MappingType destAndSrcBlock,
                AxisIndex destAndSrcIndex,
                bool invert = false) 
                : this(destAndSrcBlock, (UInt32)destAndSrcIndex, destAndSrcBlock, (UInt32)destAndSrcIndex, invert)
            {
            }

            public DeviceMapping(
                MappingType destAndSrcBlock,
                UInt32 destAndSrcIndex,
                bool invert = false)
                : this(destAndSrcBlock, destAndSrcIndex, destAndSrcBlock, destAndSrcIndex, invert)
            {
            }

            public DeviceMapping(RegistryKey k)
            {
                var enumType = typeof(MappingType);
                this.destBlock = (MappingType)Enum.Parse(enumType, k.GetValue(ValueDestBlock).ToString());
                this.destIndex = Convert.ToUInt32(k.GetValue(ValueDestIndex));
                this.srcBlock = (MappingType)Enum.Parse(enumType, k.GetValue(ValueSrcBlock).ToString());
                this.srcIndex = Convert.ToUInt32(k.GetValue(ValueSrcIndex));
                this.invert = Convert.ToInt32(k.GetValue(ValueInvert));
            }

            public void WriteToRegistry(RegistryKey k)
            {
                k.SetValue(ValueDestBlock, this.destBlock, RegistryValueKind.String);
                k.SetValue(ValueDestIndex, this.destIndex, RegistryValueKind.DWord);
                k.SetValue(ValueSrcBlock, this.srcBlock, RegistryValueKind.String);
                k.SetValue(ValueSrcIndex, this.srcIndex, RegistryValueKind.DWord);
                k.SetValue(ValueInvert, this.invert, RegistryValueKind.DWord);
            }

            public void WriteToRegistry(RegistryKey k, string subkeyName)
            {
                using (var subkey = k.CreateSubKey(subkeyName, true))
                {
                    this.WriteToRegistry(subkey);
                }
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
        public static extern bool SetDeviceMapping(
            UInt32 h,
            [MarshalAs(UnmanagedType.LPStr)] string deviceGUID,
            DeviceMapping[] mappings, 
            Int32 mappingCount);

        public delegate void DeviceEnumCallback(
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string guid);

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "EnumerateDevices",
            SetLastError = false)]
        public static extern bool EnumerateDevices(UInt32 h, DeviceEnumCallback cb);
    }
}