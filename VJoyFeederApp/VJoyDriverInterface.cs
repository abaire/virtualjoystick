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
            private MappingType _virtualDeviceType; //!< The type of the target virtual joystick state
            private UInt32 _virtualDeviceIndex; //!< The index of the target virtual joystick state

            private MappingType _sourceType; //!< The type of the source state
            private UInt32 _sourceIndex; //!< The index of the source joystick state

            private Int32 _invert;

            public DeviceMapping(
                MappingType virtualDeviceType,
                UInt32 virtualDeviceIndex,
                MappingType sourceType,
                UInt32 sourceIndex,
                bool invert = false)
            {
                this._virtualDeviceType = virtualDeviceType;
                this._virtualDeviceIndex = virtualDeviceIndex;
                this._sourceType = sourceType;
                this._sourceIndex = sourceIndex;
                this._invert = invert ? 1 : 0;
            }

            public DeviceMapping(
                MappingType virtualDeviceType,
                AxisIndex destIndex,
                MappingType sourceType,
                AxisIndex srcIndex,
                bool invert = false)
                : this(virtualDeviceType, (UInt32) destIndex, sourceType, (UInt32) srcIndex, invert)
            {
            }

            public DeviceMapping(
                MappingType destAndSrcBlock,
                AxisIndex destAndSrcIndex,
                bool invert = false)
                : this(destAndSrcBlock, (UInt32) destAndSrcIndex, destAndSrcBlock, (UInt32) destAndSrcIndex, invert)
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
                _virtualDeviceType = (MappingType) Enum.Parse(
                    enumType,
                    k.GetValue(DeviceRegistry.REGISTRY_VALUE_VIRTUAL_DEVICE_TYPE, 0).ToString());

                _virtualDeviceIndex = ReadDeviceIndex(
                    k,
                    DeviceRegistry.REGISTRY_VALUE_VIRTUAL_DEVICE_INDEX,
                    _virtualDeviceType);


                _sourceType = (MappingType) Enum.Parse(
                    enumType,
                    k.GetValue(DeviceRegistry.REGISTRY_VALUE_SOURCE_TYPE, 0).ToString());

                _sourceIndex = ReadDeviceIndex(
                    k,
                    DeviceRegistry.REGISTRY_VALUE_SOURCE_INDEX,
                    _sourceType);


                this._invert = Convert.ToInt32(k.GetValue(DeviceRegistry.REGISTRY_VALUE_INVERT, 0));
            }

            private static UInt32 ReadDeviceIndex(RegistryKey k, string valueName, MappingType mappingType)
            {
                var value = k.GetValue(valueName, 0);

                if (mappingType == MappingType.axis)
                {
                    try
                    {
                        var kind = k.GetValueKind(valueName);
                        if (kind == RegistryValueKind.String)
                        {
                            var axisIndex = (AxisIndex) Enum.Parse(typeof(AxisIndex), value.ToString());
                            return (UInt32) axisIndex;
                        }
                    } 
                    catch (System.IO.IOException)
                    {
                        // Return a default mapping.
                        return 0;
                    }
                }

                return Convert.ToUInt32(value);
            }

            public void WriteToRegistry(RegistryKey k)
            {
                k.SetValue(DeviceRegistry.REGISTRY_VALUE_VIRTUAL_DEVICE_TYPE, _virtualDeviceType,
                    RegistryValueKind.String);
                WriteDeviceIndex(
                    k,
                    DeviceRegistry.REGISTRY_VALUE_VIRTUAL_DEVICE_INDEX,
                    _virtualDeviceIndex,
                    _virtualDeviceType);

                k.SetValue(DeviceRegistry.REGISTRY_VALUE_SOURCE_TYPE, _sourceType, RegistryValueKind.String);
                WriteDeviceIndex(
                    k,
                    DeviceRegistry.REGISTRY_VALUE_SOURCE_INDEX,
                    _sourceIndex,
                    _sourceType);

                k.SetValue(DeviceRegistry.REGISTRY_VALUE_INVERT, _invert, RegistryValueKind.DWord);
            }

            private static void WriteDeviceIndex(RegistryKey k, string valueName, UInt32 index, MappingType type)
            {
                if (type != MappingType.axis)
                {
                    k.SetValue(valueName, index, RegistryValueKind.DWord);
                }
                else
                {
                    k.SetValue(valueName, (AxisIndex) index, RegistryValueKind.String);
                }
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

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "ClearDeviceMappings",
            SetLastError = false)]
        public static extern bool ClearDeviceMappings(UInt32 h);

        public delegate void DeviceEnumCallback(
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string guid);

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "EnumerateDevices",
            SetLastError = false)]
        public static extern bool EnumerateDevices(UInt32 h, DeviceEnumCallback cb);

        public delegate void DeviceInfoCallback(
            MappingType elementType,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            UInt32 srcIndex);

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "GetDeviceInfo",
            SetLastError = false)]
        public static extern bool GetDeviceInfo(
            UInt32 h,
            [MarshalAs(UnmanagedType.LPStr)] string deviceGUID,
            DeviceInfoCallback cb);
    }
}