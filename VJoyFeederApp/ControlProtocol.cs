using System;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.DirectX.DirectInput;

namespace JoystickUsermodeDriver
{
    internal interface IControlProtocolDelegate
    {
        bool HandleKeycode(uint keycode, bool isPressed);
        bool HandleAxis(byte axis, short position);
        bool HandleButton(byte button, bool isPressed);
        bool HandlePOV(byte povState);

        bool HandleEchoRequest(bool echoOn);

        bool HandleGetCurrentState();
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    internal class Handshake
    {
        internal static byte[] ExpectedMagicBytes = Encoding.ASCII.GetBytes("bbvj");

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        internal byte[] magicBytes;

        internal byte version;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    internal struct KeycodeCommand
    {
        internal uint keycode;
        internal byte isPressed;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    internal struct ButtonCommand
    {
        internal byte buttonID;
        internal byte isPressed;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    internal struct AxisCommand
    {
        internal byte axis;
        internal short position;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    internal struct POVCommand
    {
        internal byte povState;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    internal struct EchoCommand
    {
        internal byte echoState;
    }

    internal class NetworkUtil
    {
        internal static void ShiftBuffer(ref byte[] buffer, ref int bufferLength, int bytesConsumed)
        {
            if (bufferLength <= bytesConsumed)
            {
                bufferLength = 0;
                return;
            }

            var bytesToMove = bufferLength - bytesConsumed;
            Buffer.BlockCopy(buffer, bytesConsumed, buffer, 0, bytesToMove);
            bufferLength -= bytesConsumed;
        }

        private NetworkUtil() { }
    }

    internal class ControlProtocol
    {
        private WeakReference<IControlProtocolDelegate> _delegate;

        private byte _negotiatedVersion;
        private State _state = State.WaitingForHandshake;

        internal ControlProtocol(IControlProtocolDelegate protocolDelegate)
        {
            _delegate = new WeakReference<IControlProtocolDelegate>(protocolDelegate);
        }

        internal bool Handle(ref byte[] buffer, ref int bufferLength)
        {
            bool result;
            switch (_state)
            {
                case State.WaitingForHandshake:
                    result = HandleHandshake(ref buffer, ref bufferLength);
                    break;

                case State.AcceptingCommands:
                    result = HandleCommand(ref buffer, ref bufferLength);
                    break;

                default:
                    result = false;
                    break;
            }

            return result;
        }

        private bool HandleHandshake(ref byte[] buffer, ref int bufferLength)
        {
            var packetSize = Marshal.SizeOf(typeof(Handshake));
            if (bufferLength < packetSize) return true;

            var gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            var obj = (Handshake) Marshal.PtrToStructure(gch.AddrOfPinnedObject(), typeof(Handshake));
            gch.Free();

            NetworkUtil.ShiftBuffer(ref buffer, ref bufferLength, packetSize);

            if (!obj.magicBytes.SequenceEqual(Handshake.ExpectedMagicBytes)) return false;

            _negotiatedVersion = obj.version;
            _state = State.AcceptingCommands;

            return true;
        }

        private bool HandleCommand(ref byte[] buffer, ref int bufferLength)
        {
            if (bufferLength < 1) return true;
            var cmd = (Command) buffer[0];
            if (!Enum.IsDefined(typeof(Command), cmd)) return false;

            switch (cmd)
            {
                default:
                    return false;

                case Command.Keycode:
                    return HandleKeycode(ref buffer, ref bufferLength);

                case Command.Button:
                    return HandleButton(ref buffer, ref bufferLength);

                case Command.Axis:
                    return HandleAxis(ref buffer, ref bufferLength);

                case Command.POV:
                    return HandlePOV(ref buffer, ref bufferLength);

                case Command.Echo:
                    return HandleEcho(ref buffer, ref bufferLength);

                case Command.GetCurrentState:
                    return HandleGetCurrentState(ref buffer, ref bufferLength);
            }
        }

        private bool HandleKeycode(ref byte[] buffer, ref int bufferLength)
        {
            var packetType = typeof(KeycodeCommand);
            var packetSize = Marshal.SizeOf(packetType);
            if (bufferLength < packetSize + 1) return true;

            var gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            var obj = (KeycodeCommand) Marshal.PtrToStructure(gch.AddrOfPinnedObject() + 1, packetType);
            gch.Free();

            obj.keycode = (uint) IPAddress.NetworkToHostOrder((int) obj.keycode);


            NetworkUtil.ShiftBuffer(ref buffer, ref bufferLength, packetSize + 1);

            IControlProtocolDelegate d;
            if (_delegate != null && _delegate.TryGetTarget(out d)) return d.HandleKeycode(obj.keycode, obj.isPressed != 0);
            return true;
        }

        private bool HandleButton(ref byte[] buffer, ref int bufferLength)
        {
            var packetType = typeof(ButtonCommand);
            var packetSize = Marshal.SizeOf(packetType);
            if (bufferLength < packetSize + 1) return true;

            var gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            var obj = (ButtonCommand) Marshal.PtrToStructure(gch.AddrOfPinnedObject() + 1, packetType);
            gch.Free();

            NetworkUtil.ShiftBuffer(ref buffer, ref bufferLength, packetSize + 1);

            if (obj.buttonID > 127) return false;

            IControlProtocolDelegate d;
            if (_delegate != null && _delegate.TryGetTarget(out d)) return d.HandleButton(obj.buttonID, obj.isPressed != 0);
            return true;
        }

        private bool HandleAxis(ref byte[] buffer, ref int bufferLength)
        {
            var packetType = typeof(AxisCommand);
            var packetSize = Marshal.SizeOf(packetType);
            if (bufferLength < packetSize + 1) return true;

            var gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            var obj = (AxisCommand) Marshal.PtrToStructure(gch.AddrOfPinnedObject() + 1, packetType);
            gch.Free();

            obj.position = IPAddress.NetworkToHostOrder(obj.position);
            NetworkUtil.ShiftBuffer(ref buffer, ref bufferLength, packetSize + 1);

            IControlProtocolDelegate d;
            if (_delegate != null && _delegate.TryGetTarget(out d)) return d.HandleAxis(obj.axis, obj.position);
            return true;
        }

        private bool HandlePOV(ref byte[] buffer, ref int bufferLength)
        {
            var packetType = typeof(POVCommand);
            var packetSize = Marshal.SizeOf(packetType);
            if (bufferLength < packetSize + 1) return true;

            var gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            var obj = (POVCommand) Marshal.PtrToStructure(gch.AddrOfPinnedObject() + 1, packetType);
            gch.Free();

            NetworkUtil.ShiftBuffer(ref buffer, ref bufferLength, packetSize + 1);

            if (obj.povState > 8 || obj.povState < 0) return false;

            IControlProtocolDelegate d;
            if (_delegate != null && _delegate.TryGetTarget(out d)) return d.HandlePOV(obj.povState);
            return true;
        }

        private bool HandleEcho(ref byte[] buffer, ref int bufferLength)
        {
            var packetType = typeof(EchoCommand);
            var packetSize = Marshal.SizeOf(packetType);
            if (bufferLength < packetSize + 1) return true;

            var gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            var obj = (EchoCommand)Marshal.PtrToStructure(gch.AddrOfPinnedObject() + 1, packetType);
            gch.Free();

            NetworkUtil.ShiftBuffer(ref buffer, ref bufferLength, packetSize + 1);

            IControlProtocolDelegate d;
            if (_delegate != null && _delegate.TryGetTarget(out d)) return d.HandleEchoRequest(obj.echoState != 0);
            return true;
        }

        private bool HandleGetCurrentState(ref byte[] buffer, ref int bufferLength)
        {
            if (bufferLength < 1) return true;
            NetworkUtil.ShiftBuffer(ref buffer, ref bufferLength, 1);

            IControlProtocolDelegate d;
            if (_delegate != null && _delegate.TryGetTarget(out d)) return d.HandleGetCurrentState();
            return true;
        }
        private enum State
        {
            WaitingForHandshake,
            AcceptingCommands
        }

        private enum Command : byte
        {
            Keycode = 0x10,
            Button,
            Axis,
            POV,

            Echo = 0x55,
            GetCurrentState
        }
    }
}