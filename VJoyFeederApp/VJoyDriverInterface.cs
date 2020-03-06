using Microsoft.Win32;
using System;
using System.Runtime.InteropServices;

namespace JoystickUsermodeDriver
{
    public class VJoyDriverInterface
    {
        // Keep in sync with VJoyDriverInterface.h

        public const int MaxVirtualButtons = 128;
        public const int MaxVirtualPOVs = 1;

        public enum MappingType
        {
            Axis = 0,
            POV,
            Button,
            Key
        }

        public enum AxisIndex
        {
            axis_none = (int)UNMAPPED_INDEX,
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

        // Keep in sync with VJoyDriverInterface.h
        private const UInt32 UNMAPPED_INDEX = 0xFFFF;

        private const Int32 MODIFIER_LEFT_CTRL = (1 << 0);
        private const Int32 MODIFIER_LEFT_SHIFT = (1 << 1);
        private const Int32 MODIFIER_LEFT_ALT = (1 << 2);
        private const Int32 MODIFIER_LEFT_GUI = (1 << 3);
        private const Int32 MODIFIER_RIGHT_CTRL = (1 << 4);
        private const Int32 MODIFIER_RIGHT_SHIFT = (1 << 5);
        private const Int32 MODIFIER_RIGHT_ALT = (1 << 6);
        private const Int32 MODIFIER_RIGHT_GUI = (1 << 7);

        public enum Keycode
        {
            Escape = 0x1B,
            Enter = (int)'\n',
            Backspace = 0x08,
            Tab = (int)'\t',
            Space = (int)' ',

            F1 = 0x013A,
            F2 = 0x013B,
            F3 = 0x013C,
            F4 = 0x013D,
            F5 = 0x013E,
            F6 = 0x013F,
            F7 = 0x0140,
            F8 = 0x0141,
            F9 = 0x0142,
            F10 = 0x0143,
            F11 = 0x0144,
            F12 = 0x0145,

            RightArrow = 0x014F,
            LeftArrow = 0x0150,
            DownArrow = 0x0151,
            UpArrow = 0x0152,

            KEYPAD_1 = 0x0159,
            KEYPAD_2 = 0x015A,
            KEYPAD_3 = 0x015B,
            KEYPAD_4 = 0x015C,
            KEYPAD_5 = 0x015D,
            KEYPAD_6 = 0x015E,
            KEYPAD_7 = 0x015F,
            KEYPAD_8 = 0x0160,
            KEYPAD_9 = 0x0161,
            KEYPAD_0 = 0x0162,

            LEFT_CTRL = (MODIFIER_LEFT_CTRL << 16),
            LEFT_SHIFT = (MODIFIER_LEFT_SHIFT << 16),
            LEFT_ALT = (MODIFIER_LEFT_ALT << 16),
            LEFT_GUI = (MODIFIER_LEFT_GUI << 16),
            RIGHT_CTRL = (MODIFIER_RIGHT_CTRL << 16),
            RIGHT_SHIFT = (MODIFIER_RIGHT_SHIFT << 16),
            RIGHT_ALT = (MODIFIER_RIGHT_ALT << 16),
            RIGHT_GUI = (MODIFIER_RIGHT_GUI << 16),
        }

        [StructLayout(LayoutKind.Sequential)]
        public class VirtualDeviceState
        {
            public Int16 X;
            public Int16 Y;

            public Int16 throttle;
            public Int16 rudder;

            public Int16 rX;
            public Int16 rY;
            public Int16 rZ;

            public Int16 slider;
            public Int16 dial;

            public bool povNorth;
            public bool povEast;
            public bool povSouth;
            public bool povWest;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
            public byte[] button; // 128 button bits

            public byte modifierKeys;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 7)]
            public UInt32[] keycodes;

            public VirtualDeviceState()
            {
                button = new byte[16];
                keycodes = new UInt32[7];
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct DeviceMapping
        {
            public const UInt32 Unmapped = (UInt32) AxisIndex.axis_none;

            private MappingType _virtualDeviceType; //!< The type of the target virtual joystick state
            private UInt32 _virtualDeviceIndex; //!< The index of the target virtual joystick state

            private MappingType _sourceType; //!< The type of the source state
            private UInt32 _sourceIndex; //!< The index of the source joystick state

            private Int32 _invert;

            public MappingType VirtualDeviceType => _virtualDeviceType;

            public UInt32 VirtualDeviceIndex => _virtualDeviceIndex;

            public string VirtualDeviceName
            {
                get
                {
                    if (_virtualDeviceType == MappingType.Axis)
                    {
                        return VirtualDeviceIndexName;
                    }

                    if (_sourceType == MappingType.POV && _virtualDeviceType == MappingType.Button)
                    {
                        return $"Buttons {VirtualDeviceIndex + 1} - {VirtualDeviceIndex + 4}";
                    }

                    if (_virtualDeviceType == MappingType.Key)
                    {
                        return $"{(char) (VirtualDeviceIndex & 0xFF)}";
                    }

                    return $"{VirtualDeviceType} {VirtualDeviceIndexName}";
                }
            }

            public string VirtualDeviceIndexName => IndexName(_virtualDeviceType, _virtualDeviceIndex);

            public MappingType SourceType => _sourceType;

            public string SourceName
            {
                get
                {
                    if (_sourceType == MappingType.Axis)
                    {
                        return SourceIndexName;
                    }

                    return $"{SourceType} {SourceIndexName}";
                }
            }

            public string SourceIndexName => IndexName(_sourceType, _sourceIndex);

            public UInt32 SourceIndex => _sourceIndex;

            public bool Invert => _invert != 0;

            private string IndexName(MappingType type, UInt32 index)
            {
                if (type == MappingType.Axis)
                {
                    AxisIndex axisIndex = (AxisIndex) index;
                    return axisIndex.ToString();
                }

                return index.ToString();
            }

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

                if (mappingType == MappingType.Axis)
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
                        return (UInt32)AxisIndex.axis_none;
                    }
                }

                if (mappingType == MappingType.Key)
                {
                    try
                    {
                        var kind = k.GetValueKind(valueName);
                        if (kind == RegistryValueKind.String)
                        {
                            UInt32 keycode = 0x04;
                            var stringVal = value.ToString();
                            // Single char strings are assumed to be printable and mapped to ASCII.
                            if (stringVal.Length == 1)
                            {
                                keycode = (UInt32)stringVal[0];
                                if (keycode < 0x80)
                                {
                                    return keycode;
                                }

                                return 0x04;
                            }
                            keycode = (UInt32)(Keycode)Enum.Parse(typeof(Keycode), value.ToString());
                            return (UInt32)keycode;
                        }
                    }
                    catch (System.IO.IOException)
                    {
                        // Return a default mapping.
                        return 0x04;
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
                if (type != MappingType.Axis)
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

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "UpdateLoopDelay",
            SetLastError = false)]
        public static extern UInt32 UpdateLoopDelay(UInt32 h);

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "SetUpdateLoopDelay",
            SetLastError = false)]
        public static extern bool SetUpdateLoopDelay(UInt32 h, UInt32 delay);

        [System.Runtime.InteropServices.DllImport("VJoyDirectXBridge.dll", EntryPoint = "SetVirtualDeviceState",
            SetLastError = false)]
        public static extern bool SetVirtualDeviceState(UInt32 h, VirtualDeviceState state, bool allowOverride);
    }
}