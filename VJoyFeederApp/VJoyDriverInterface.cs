using System;
using System.IO;
using System.Net;
using System.Runtime.InteropServices;
using Microsoft.DirectX.DirectInput;
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

        public enum TransformType: byte
        {
            None = 0,
            InvertAxis,
            RapidFire,
            EdgeDetect
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
        public static extern bool SetVirtualDeviceState(uint h, VirtualDeviceState state, bool allowOverride = true);

        // Keep in sync with struct _VirtualDeviceState in VJoyDriverInterface.h.
        [StructLayout(LayoutKind.Explicit, Pack = 1)]
        public class VirtualDeviceState
        {
            public const int MaxSimultaneousKeys = 7;

            // Ordering must match the AxisIndex for serialization.
            [FieldOffset(0)]
            public short X;
            [FieldOffset(2)]
            public short Y;
            [FieldOffset(4)]
            public short Throttle;
            [FieldOffset(6)]
            public short RX;
            [FieldOffset(8)]
            public short RY;
            [FieldOffset(10)]
            public short RZ;
            [FieldOffset(12)]
            public short Slider;
            [FieldOffset(14)]
            public short Dial;
            [FieldOffset(16)]
            public short Rudder;

            [MarshalAs(UnmanagedType.I1)]
            [FieldOffset(20)]
            public bool POVNorth;
            [MarshalAs(UnmanagedType.I1)]
            [FieldOffset(21)]
            public bool POVEast;
            [MarshalAs(UnmanagedType.I1)]
            [FieldOffset(22)]
            public bool POVSouth;
            [MarshalAs(UnmanagedType.I1)]
            [FieldOffset(23)]
            public bool POVWest;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
            [FieldOffset(24)]
            public byte[] Button; // 128 button bits

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = MaxSimultaneousKeys, ArraySubType = UnmanagedType.U4)]
            [FieldOffset(40)]
            public UInt32[] Keycodes;

            [FieldOffset(68)]
            public byte ModifierKeys;

            public VirtualDeviceState()
            {
                Button = new byte[16];
                Keycodes = new uint[7];
            }

            public VirtualDeviceState NetworkOrdered()
            {
                var ret = new VirtualDeviceState();
                ret.X = IPAddress.HostToNetworkOrder(X);
                ret.Y = IPAddress.HostToNetworkOrder(Y);
                ret.Throttle = IPAddress.HostToNetworkOrder(Throttle);
                ret.RX = IPAddress.HostToNetworkOrder(RX);
                ret.RY = IPAddress.HostToNetworkOrder(RY);
                ret.RZ = IPAddress.HostToNetworkOrder(RZ);
                ret.Slider = IPAddress.HostToNetworkOrder(Slider);
                ret.Dial = IPAddress.HostToNetworkOrder(Dial);
                ret.Rudder = IPAddress.HostToNetworkOrder(Rudder);

                ret.POVNorth = POVNorth;
                ret.POVEast = POVEast;
                ret.POVSouth = POVSouth;
                ret.POVWest = POVWest;

                Buffer.BlockCopy(Button, 0, ret.Button, 0, 16);
                ret.ModifierKeys = ModifierKeys;

                for (var i = 0; i < MaxSimultaneousKeys; ++i)
                {
                    ret.Keycodes[i] = (uint)IPAddress.NetworkToHostOrder((int)Keycodes[i]);
                }

                return ret;
            }

            public bool SetButton(byte buttonNumber, bool isOn = true)
            {
                if (buttonNumber > 128) return false;

                var offset = buttonNumber >> 3;
                var bit = (byte) (1 << (buttonNumber & 0x07));

                if (isOn)
                    Button[offset] |= bit;
                else
                    Button[offset] &= (byte) ~bit;

                return true;
            }

            public bool SetAxis(int axis, short val)
            {
                var axisIndex = (AxisIndex) axis;
                if (!Enum.IsDefined(typeof(AxisIndex), axisIndex)) return false;
                return SetAxis(axisIndex, val);
            }

            public bool SetAxis(AxisIndex axis, short val)
            {
                if (axis == AxisIndex.axis_none) return true;

                switch (axis)
                {
                    default:
                        return false;

                    case AxisIndex.axis_x:
                        X = val;
                        break;

                    case AxisIndex.axis_y:
                        Y = val;
                        break;

                    case AxisIndex.axis_throttle: 
                        Throttle = val;
                        break;

                    case AxisIndex.axis_rudder:
                        Rudder = val;
                        break;

                    case AxisIndex.axis_rx:
                        RX = val;
                        break;

                    case AxisIndex.axis_ry:
                        RY = val;
                        break;

                    case AxisIndex.axis_rz:
                        RZ = val;
                        break;

                    case AxisIndex.axis_slider:
                        Slider = val;
                        break;

                    case AxisIndex.axis_dial:
                        Dial = val;
                        break;
                }

                return true;
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

            public void ClearPOV()
            {
                POVEast = POVNorth = POVWest = POVSouth = false;
            }

            public void ClearMaskedModifierKeys(int mask)
            {
                ModifierKeys = (byte) (ModifierKeys & mask);
            }
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct DeviceMapping
        {
            public const uint Unmapped = (uint)AxisIndex.axis_none;

            [MarshalAs(UnmanagedType.U4)]
            public MappingType VirtualDeviceType;
            public uint VirtualDeviceIndex;
            [MarshalAs(UnmanagedType.U4)]
            public MappingType SourceType;
            public uint SourceIndex;
            [MarshalAs(UnmanagedType.U1)]
            public TransformType Transform;
            public byte DownMillis;
            public byte RepeatMillis;
            public byte SensitivityBoost;

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

            public string SourceName
            {
                get
                {
                    if (SourceType == MappingType.Axis) return SourceIndexName;

                    return $"{SourceType} {SourceIndexName}";
                }
            }

            public string SourceIndexName => IndexName(SourceType, SourceIndex);

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
                TransformType transform = TransformType.None,
                byte downMillis = 0,
                byte repeatMillis = 0,
                byte sensitivityBoost = 0)
            {
                VirtualDeviceType = virtualDeviceType;
                VirtualDeviceIndex = virtualDeviceIndex;
                SourceType = sourceType;
                SourceIndex = sourceIndex;
                Transform = transform;
                DownMillis = downMillis;
                RepeatMillis = repeatMillis;
                SensitivityBoost = sensitivityBoost;
            }

            public DeviceMapping(
                MappingType virtualDeviceType,
                AxisIndex destIndex,
                MappingType sourceType,
                AxisIndex srcIndex,
                TransformType transform = TransformType.None,
                byte downMillis = 0,
                byte repeatMillis = 0,
                byte sensitivityBoost = 0)
                : this(
                    virtualDeviceType,
                    (uint) destIndex, 
                    sourceType, 
                    (uint) srcIndex,
                    transform, 
                    downMillis, 
                    repeatMillis, 
                    sensitivityBoost)
            {
            }

            public DeviceMapping(
                MappingType destAndSrcMappingType,
                AxisIndex destAndSrcIndex,
                TransformType transform = TransformType.None,
                byte downMillis = 0,
                byte repeatMillis = 0,
                byte sensitivityBoost = 0)
                : this(
                    destAndSrcMappingType,
                    (uint) destAndSrcIndex, 
                    destAndSrcMappingType, 
                    (uint) destAndSrcIndex,
                    transform,
                    downMillis,
                    repeatMillis,
                    sensitivityBoost)
            {
            }

            public DeviceMapping(
                MappingType destAndSrcMappingType,
                uint destAndSrcIndex,
                TransformType transform = TransformType.None,
                byte downMillis = 0,
                byte repeatMillis = 0,
                byte sensitivityBoost = 0) 
                : this(
                    destAndSrcMappingType, 
                    destAndSrcIndex, 
                    destAndSrcMappingType, 
                    destAndSrcIndex,
                    transform,
                    downMillis,
                    repeatMillis,
                    sensitivityBoost)
            {
            }

        }
    }
}