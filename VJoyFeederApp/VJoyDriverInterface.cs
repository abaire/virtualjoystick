using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32;

namespace JoystickUsermodeDriver
{
    public class VJoyDriverInterface
    {
        public delegate void DeviceEnumCallback(
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string guid);

        public delegate void DeviceInfoCallback(
            MappingType elementType,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            uint srcIndex);

        public enum AxisIndex
        {
            axis_none = (int) UNMAPPED_INDEX,
            axis_x = 0,
            axis_y,
            axis_throttle,
            axis_rx,
            axis_ry,
            axis_rz,
            axis_slider,
            axis_dial,

            // Additional axes are not available in DIJOYSTATE2.
            axis_rudder
        }

        public enum Keycode
        {
            Escape = 0x1B,
            Enter = (int) '\n',
            Backspace = 0x08,
            Tab = (int) '\t',
            Space = (int) ' ',

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

            Keypad_1 = 0x0159,
            Keypad_2 = 0x015A,
            Keypad_3 = 0x015B,
            Keypad_4 = 0x015C,
            Keypad_5 = 0x015D,
            Keypad_6 = 0x015E,
            Keypad_7 = 0x015F,
            Keypad_8 = 0x0160,
            Keypad_9 = 0x0161,
            Keypad_0 = 0x0162,

            LeftCtrl = MODIFIER_LEFT_CTRL << 16,
            LeftShift = MODIFIER_LEFT_SHIFT << 16,
            LeftAlt = MODIFIER_LEFT_ALT << 16,
            LeftGUI = MODIFIER_LEFT_GUI << 16,
            RightCtrl = MODIFIER_RIGHT_CTRL << 16,
            RightShift = MODIFIER_RIGHT_SHIFT << 16,
            RightAlt = MODIFIER_RIGHT_ALT << 16,
            RightGUI = MODIFIER_RIGHT_GUI << 16
        }

        public enum MappingType
        {
            Axis = 0,
            POV,
            Button,
            Key
        }
        // Keep in sync with VJoyDriverInterface.h

        public const int MaxVirtualButtons = 128;
        public const int MaxVirtualPOVs = 1;

        // Keep in sync with VJoyDriverInterface.h
        private const uint UNMAPPED_INDEX = 0xFFFF;

        private const int MODIFIER_LEFT_CTRL = 1 << 0;
        private const int MODIFIER_LEFT_SHIFT = 1 << 1;
        private const int MODIFIER_LEFT_ALT = 1 << 2;
        private const int MODIFIER_LEFT_GUI = 1 << 3;
        private const int MODIFIER_RIGHT_CTRL = 1 << 4;
        private const int MODIFIER_RIGHT_SHIFT = 1 << 5;
        private const int MODIFIER_RIGHT_ALT = 1 << 6;
        private const int MODIFIER_RIGHT_GUI = 1 << 7;

        public static uint INVALID_HANDLE_VALUE = 0xFFFFFFFF;

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "AttachToVirtualJoystickDriver",
            SetLastError = false)]
        public static extern uint AttachToVirtualJoystickDriver();

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "BeginDriverUpdateLoop",
            SetLastError = false)]
        public static extern bool BeginDriverUpdateLoop(uint h);

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "EndDriverUpdateLoop",
            SetLastError = false)]
        public static extern bool EndDriverUpdateLoop(uint h);

        [DllImport("VJoyDirectXBridge.dll",
            EntryPoint = "DetachFromVirtualJoystickDriver", SetLastError = false)]
        public static extern bool DetachFromVirtualJoystickDriver(uint h);

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "SetDeviceMapping",
            SetLastError = false)]
        public static extern bool SetDeviceMapping(
            uint h,
            [MarshalAs(UnmanagedType.LPStr)] string deviceGUID,
            DeviceMapping[] mappings,
            int mappingCount);

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "ClearDeviceMappings",
            SetLastError = false)]
        public static extern bool ClearDeviceMappings(uint h);

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "EnumerateDevices",
            SetLastError = false)]
        public static extern bool EnumerateDevices(uint h, DeviceEnumCallback cb);

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "GetDeviceInfo",
            SetLastError = false)]
        public static extern bool GetDeviceInfo(
            uint h,
            [MarshalAs(UnmanagedType.LPStr)] string deviceGUID,
            DeviceInfoCallback cb);

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "UpdateLoopDelay",
            SetLastError = false)]
        public static extern uint UpdateLoopDelay(uint h);

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "SetUpdateLoopDelay",
            SetLastError = false)]
        public static extern bool SetUpdateLoopDelay(uint h, uint delay);

        [DllImport("VJoyDirectXBridge.dll", EntryPoint = "SetVirtualDeviceState",
            SetLastError = false)]
        public static extern bool SetVirtualDeviceState(uint h, VirtualDeviceState state, bool allowOverride);

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public class VirtualDeviceState
        {
            private const int MaxSimultaneousKeys = 7;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
            public byte[] Button; // 128 button bits

            public short Dial;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = MaxSimultaneousKeys)]
            public uint[] Keycodes;

            public byte ModifierKeys;
            public bool POVEast;

            public bool POVNorth;
            public bool POVSouth;
            public bool POVWest;
            public short Rudder;

            public short RX;
            public short RY;
            public short RZ;

            public short Slider;

            public short Throttle;

            public short X;
            public short Y;

            public VirtualDeviceState()
            {
                Button = new byte[16];
                Keycodes = new uint[7];
            }

            public void SetButton(byte buttonNumber, bool isOn = true)
            {
                if (buttonNumber > 128) return;

                var offset = buttonNumber >> 3;
                var bit = (byte) (1 << (buttonNumber & 0x07));

                if (isOn)
                    Button[offset] |= bit;
                else
                    Button[offset] &= (byte) ~bit;
            }

            public bool SetKey(char key, bool isDown = true)
            {
                return SetKey((uint) key, isDown);
            }

            public bool SetKey(Keycode key, bool isDown = true)
            {
                return SetKey((uint) key, isDown);
            }

            public bool SetKey(uint key, bool isDown = true)
            {
                if (isDown)
                {
                    for (var i = 0; i < MaxSimultaneousKeys; ++i)
                        if (Keycodes[i] == 0)
                        {
                            Keycodes[i] = key;
                            return true;
                        }
                }
                else
                {
                    for (var i = 0; i < MaxSimultaneousKeys; ++i)
                        if (Keycodes[i] == key)
                        {
                            Keycodes[i] = 0;
                            return true;
                        }
                }

                return false;
            }
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct DeviceMapping
        {
            public const uint Unmapped = (uint) AxisIndex.axis_none;

            private readonly int _invert;

            public MappingType VirtualDeviceType { get; }

            public uint VirtualDeviceIndex { get; }

            public string VirtualDeviceName
            {
                get
                {
                    if (VirtualDeviceType == MappingType.Axis) return VirtualDeviceIndexName;

                    if (SourceType == MappingType.POV && VirtualDeviceType == MappingType.Button)
                        return $"Buttons {VirtualDeviceIndex + 1} - {VirtualDeviceIndex + 4}";

                    if (VirtualDeviceType == MappingType.Key) return $"{(char) (VirtualDeviceIndex & 0xFF)}";

                    return $"{VirtualDeviceType} {VirtualDeviceIndexName}";
                }
            }

            public string VirtualDeviceIndexName => IndexName(VirtualDeviceType, VirtualDeviceIndex);

            public MappingType SourceType { get; }

            public string SourceName
            {
                get
                {
                    if (SourceType == MappingType.Axis) return SourceIndexName;

                    return $"{SourceType} {SourceIndexName}";
                }
            }

            public string SourceIndexName => IndexName(SourceType, SourceIndex);

            public uint SourceIndex { get; }

            public bool Invert => _invert != 0;

            private string IndexName(MappingType type, uint index)
            {
                if (type == MappingType.Axis)
                {
                    var axisIndex = (AxisIndex) index;
                    return axisIndex.ToString();
                }

                return index.ToString();
            }

            public DeviceMapping(
                MappingType virtualDeviceType,
                uint virtualDeviceIndex,
                MappingType sourceType,
                uint sourceIndex,
                bool invert = false)
            {
                VirtualDeviceType = virtualDeviceType;
                VirtualDeviceIndex = virtualDeviceIndex;
                SourceType = sourceType;
                SourceIndex = sourceIndex;
                _invert = invert ? 1 : 0;
            }

            public DeviceMapping(
                MappingType virtualDeviceType,
                AxisIndex destIndex,
                MappingType sourceType,
                AxisIndex srcIndex,
                bool invert = false)
                : this(virtualDeviceType, (uint) destIndex, sourceType, (uint) srcIndex, invert)
            {
            }

            public DeviceMapping(
                MappingType destAndSrcBlock,
                AxisIndex destAndSrcIndex,
                bool invert = false)
                : this(destAndSrcBlock, (uint) destAndSrcIndex, destAndSrcBlock, (uint) destAndSrcIndex, invert)
            {
            }

            public DeviceMapping(
                MappingType destAndSrcBlock,
                uint destAndSrcIndex,
                bool invert = false)
                : this(destAndSrcBlock, destAndSrcIndex, destAndSrcBlock, destAndSrcIndex, invert)
            {
            }

            public DeviceMapping(RegistryKey k)
            {
                var enumType = typeof(MappingType);
                VirtualDeviceType = (MappingType) Enum.Parse(
                    enumType,
                    k.GetValue(DeviceRegistry.REGISTRY_VALUE_VIRTUAL_DEVICE_TYPE, 0).ToString(),
                    true);

                VirtualDeviceIndex = ReadDeviceIndex(
                    k,
                    DeviceRegistry.REGISTRY_VALUE_VIRTUAL_DEVICE_INDEX,
                    VirtualDeviceType);


                SourceType = (MappingType) Enum.Parse(
                    enumType,
                    k.GetValue(DeviceRegistry.REGISTRY_VALUE_SOURCE_TYPE, 0).ToString(),
                    true);

                SourceIndex = ReadDeviceIndex(
                    k,
                    DeviceRegistry.REGISTRY_VALUE_SOURCE_INDEX,
                    SourceType);


                _invert = Convert.ToInt32(k.GetValue(DeviceRegistry.REGISTRY_VALUE_INVERT, 0));
            }

            private static uint ReadDeviceIndex(RegistryKey k, string valueName, MappingType mappingType)
            {
                var value = k.GetValue(valueName, 0);

                if (mappingType == MappingType.Axis)
                    try
                    {
                        var kind = k.GetValueKind(valueName);
                        if (kind == RegistryValueKind.String)
                        {
                            var axisIndex = (AxisIndex) Enum.Parse(typeof(AxisIndex), value.ToString(), true);
                            return (uint) axisIndex;
                        }
                    }
                    catch (IOException)
                    {
                        // Return a default mapping.
                        return (uint) AxisIndex.axis_none;
                    }

                if (mappingType == MappingType.Key)
                    try
                    {
                        var kind = k.GetValueKind(valueName);
                        if (kind == RegistryValueKind.String)
                        {
                            uint keycode = 0x04;
                            var stringVal = value.ToString();
                            // Single char strings are assumed to be printable and mapped to ASCII.
                            if (stringVal.Length == 1)
                            {
                                keycode = stringVal[0];
                                if (keycode < 0x80) return keycode;

                                return 0x04;
                            }

                            keycode = (uint) (Keycode) Enum.Parse(typeof(Keycode), value.ToString(), true);
                            return keycode;
                        }
                    }
                    catch (IOException)
                    {
                        // Return a default mapping.
                        return 0x04;
                    }

                return Convert.ToUInt32(value);
            }

            public void WriteToRegistry(RegistryKey k)
            {
                k.SetValue(DeviceRegistry.REGISTRY_VALUE_VIRTUAL_DEVICE_TYPE, VirtualDeviceType,
                    RegistryValueKind.String);
                WriteDeviceIndex(
                    k,
                    DeviceRegistry.REGISTRY_VALUE_VIRTUAL_DEVICE_INDEX,
                    VirtualDeviceIndex,
                    VirtualDeviceType);

                k.SetValue(DeviceRegistry.REGISTRY_VALUE_SOURCE_TYPE, SourceType, RegistryValueKind.String);
                WriteDeviceIndex(
                    k,
                    DeviceRegistry.REGISTRY_VALUE_SOURCE_INDEX,
                    SourceIndex,
                    SourceType);

                k.SetValue(DeviceRegistry.REGISTRY_VALUE_INVERT, _invert, RegistryValueKind.DWord);
            }

            private static void WriteDeviceIndex(RegistryKey k, string valueName, uint index, MappingType type)
            {
                if (type != MappingType.Axis)
                    k.SetValue(valueName, index, RegistryValueKind.DWord);
                else
                    k.SetValue(valueName, (AxisIndex) index, RegistryValueKind.String);
            }

            public void WriteToRegistry(RegistryKey k, string subkeyName)
            {
                using (var subkey = k.CreateSubKey(subkeyName, true))
                {
                    WriteToRegistry(subkey);
                }
            }
        }
    }
}