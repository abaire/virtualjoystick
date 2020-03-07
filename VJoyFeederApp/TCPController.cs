using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;

namespace JoystickUsermodeDriver
{
    internal interface ICloseDelegate
    {
        void ControllerClientClosed(TCPControllerClient client);
    }

    internal interface IControlStateWatcher
    {
        bool StateUpdated(
            in VJoyDriverInterface.VirtualDeviceState state,
            in VJoyDriverInterface.VirtualDeviceState stateMask);
    }

    public interface IVirtualDeviceStateWatcher
    {
        void StateUpdated(in VJoyDriverInterface.VirtualDeviceState state);
    }

    public class TCPController : ICloseDelegate, IControlStateWatcher
    {
        private const int MaxClients = 10;
        private readonly List<TCPControllerClient> _clients = new List<TCPControllerClient>(MaxClients);

        private readonly short _port = 13057;
        private TcpListener _listener;

        private WeakReference<IVirtualDeviceStateWatcher> _stateDelegate;
        private VJoyDriverInterface.VirtualDeviceState _mergedState = new VJoyDriverInterface.VirtualDeviceState();

        public string Address;

        public IVirtualDeviceStateWatcher StateDelegate
        {
            set
            {
                _stateDelegate = new WeakReference<IVirtualDeviceStateWatcher>(value);
            }
        }

        void ICloseDelegate.ControllerClientClosed(TCPControllerClient client)
        {
            Unmerge(client.State, client.StateMask);
            _clients.Remove(client);
        }

        bool IControlStateWatcher.StateUpdated(
            in VJoyDriverInterface.VirtualDeviceState state,
            in VJoyDriverInterface.VirtualDeviceState stateMask)
        {
            Merge(state, stateMask);
            IVirtualDeviceStateWatcher d;
            if (_stateDelegate != null && _stateDelegate.TryGetTarget(out d)) d.StateUpdated(_mergedState);
            return true;
        }

        private void Unmerge(
            in VJoyDriverInterface.VirtualDeviceState state,
            in VJoyDriverInterface.VirtualDeviceState stateMask)
        {
            if (stateMask.X != 0) _mergedState.X = 0;
            if (stateMask.Y != 0) _mergedState.Y = 0;
            if (stateMask.Throttle != 0) _mergedState.Throttle = 0;
            if (stateMask.Rudder != 0) _mergedState.Rudder = 0;
            if (stateMask.RX != 0) _mergedState.RX = 0;
            if (stateMask.RY != 0) _mergedState.RY = 0;
            if (stateMask.RZ != 0) _mergedState.RZ = 0;
            if (stateMask.Slider != 0) _mergedState.Slider = 0;
            if (stateMask.Dial != 0) _mergedState.Dial = 0;

            if (stateMask.POVNorth) _mergedState.ClearPOV();

            foreach (var keycode in state.Keycodes)
            {
                if (keycode == 0) continue;
                _mergedState.SetKey(keycode, false);
            }
        }

        private void Merge(
            in VJoyDriverInterface.VirtualDeviceState state,
            in VJoyDriverInterface.VirtualDeviceState stateMask)
        {
            if (stateMask.X != 0) _mergedState.X = state.X;
            if (stateMask.Y != 0) _mergedState.Y = state.Y;
            if (stateMask.Throttle != 0) _mergedState.Throttle = state.Throttle;
            if (stateMask.Rudder != 0) _mergedState.Rudder = state.Rudder;
            if (stateMask.RX != 0) _mergedState.RX = state.RX;
            if (stateMask.RY != 0) _mergedState.RY = state.RY;
            if (stateMask.RZ != 0) _mergedState.RZ = state.RZ;
            if (stateMask.Slider != 0) _mergedState.Slider = state.Slider;
            if (stateMask.Dial != 0) _mergedState.Dial = state.Dial;

            if (stateMask.POVNorth)
            {
                _mergedState.POVNorth = state.POVNorth;
                _mergedState.POVEast = state.POVEast;
                _mergedState.POVSouth = state.POVSouth;
                _mergedState.POVWest = state.POVWest;
            }

            foreach (var keycode in state.Keycodes)
            {
                if (keycode == 0) continue;
                _mergedState.SetKey(keycode);
            }
        }

        public void Start()
        {
            Stop();

            IPAddress addr;
            if (Address != null)
                addr = IPAddress.Parse(Address);
            else
                addr = IPAddress.Any;

            _listener = new TcpListener(addr, _port);
            _listener.Start();

            _listener.BeginAcceptTcpClient(AcceptClient, this);
        }

        public void Stop()
        {
            if (_listener == null) return;
            _listener.Stop();
            _listener = null;
        }

        private static void AcceptClient(IAsyncResult ar)
        {
            var self = (TCPController) ar.AsyncState;
            if (self._listener == null) return;

            var client = self._listener.EndAcceptTcpClient(ar);

            if (self._clients.Count >= MaxClients)
            {
                client.Close();
            }
            else
            {
                var controller = new TCPControllerClient(client, self, self);
                self._clients.Add(controller);
                controller.Start();
            }

            self._listener.BeginAcceptTcpClient(AcceptClient, self);
        }
    }

    internal class TCPControllerClient : IControlProtocolDelegate
    {
        private const int BufferSize = 256;
        private readonly TcpClient _client;
        private readonly WeakReference<ICloseDelegate> _closeDelegate;
        private readonly WeakReference<IControlStateWatcher> _stateDelegate;
        private readonly ControlProtocol _protocol;
        private byte[] _buffer;
        private int _bufferWriteHead;
        private readonly VJoyDriverInterface.VirtualDeviceState _state = new VJoyDriverInterface.VirtualDeviceState();
        private readonly VJoyDriverInterface.VirtualDeviceState _stateMask = new VJoyDriverInterface.VirtualDeviceState();

        internal VJoyDriverInterface.VirtualDeviceState State => _state;
        internal VJoyDriverInterface.VirtualDeviceState StateMask => _stateMask;

        internal TCPControllerClient(TcpClient client, ICloseDelegate closeDelegate, IControlStateWatcher stateDelegate)
        {
            _client = client;
            _buffer = new byte[BufferSize];
            _protocol = new ControlProtocol(this);
            _closeDelegate = new WeakReference<ICloseDelegate>(closeDelegate);
            _stateDelegate = new WeakReference<IControlStateWatcher>(stateDelegate);
        }

        internal void Start()
        {
            try
            {
                _client.GetStream().BeginRead(_buffer, 0, BufferSize, ReadData, this);
            }
            catch (IOException)
            {
                NotifyClosed();
            }
        }

        public void Close()
        {
            _client.Close();
            NotifyClosed();
        }

        private void NotifyClosed()
        {
            ICloseDelegate d;
            if (_closeDelegate.TryGetTarget(out d)) d.ControllerClientClosed(this);
        }

        private bool NotifyStateChanged()
        {
            IControlStateWatcher d;
            if (_stateDelegate.TryGetTarget(out d)) return d.StateUpdated(_state, _stateMask);

            return true;
        }

        private bool HandleData()
        {
            var previousBytesAvailable = -1;
            while (_bufferWriteHead != previousBytesAvailable)
            {
                previousBytesAvailable = _bufferWriteHead;
                if (!_protocol.Handle(ref _buffer, ref _bufferWriteHead))
                {
                    Close();
                    return false;
                }

                // If a command was consumed, send an ack reply.
                if (_bufferWriteHead != previousBytesAvailable) _client.GetStream().WriteByte(0x01);
            }

            return true;
        }

        private static void ReadData(IAsyncResult ar)
        {
            var self = (TCPControllerClient) ar.AsyncState;
            if (!self._client.Connected)
            {
                self.NotifyClosed();
                return;
            }

            var stream = self._client.GetStream();
            try
            {
                var bytesRead = stream.EndRead(ar);
                if (bytesRead == 0)
                {
                    self.NotifyClosed();
                    return;
                }

                self._bufferWriteHead += bytesRead;
                if (!self.HandleData()) return;

                var bufferAvailable = BufferSize - self._bufferWriteHead;
                if (bufferAvailable == 0)
                {
                    self.Close();
                    return;
                }
                stream.BeginRead(self._buffer, self._bufferWriteHead, bufferAvailable, ReadData,
                    self);
            }
            catch (IOException)
            {
                self.NotifyClosed();
            }
            catch (ObjectDisposedException)
            {
                self.NotifyClosed();
            }
        }

        bool IControlProtocolDelegate.HandleKeycode(uint keycode, bool isPressed)
        {
            if (!_state.SetKey(keycode, isPressed)) return false;
            return NotifyStateChanged();
        }

        bool IControlProtocolDelegate.HandleAxis(byte axis, short position)
        {
            _stateMask.SetAxis(axis, 1);
            if (!_state.SetAxis(axis, position)) return false;
            return NotifyStateChanged();
        }

        bool IControlProtocolDelegate.HandleButton(byte button, bool isPressed)
        {
            _stateMask.SetButton(button);
            if (!_state.SetButton(button, isPressed)) return false;
            return NotifyStateChanged();
        }

        bool IControlProtocolDelegate.HandlePOV(byte povState)
        {
            _stateMask.POVEast = _stateMask.POVNorth = _stateMask.POVWest = _stateMask.POVSouth = true;
            _state.ClearPOV();
            switch (povState)
            {
                default:
                    return false;

                case 0:
                    // State is already cleared.
                    break;

                case 1:
                    _state.POVNorth = true;
                    break;

                case 2:
                    _state.POVNorth = _state.POVEast = true;
                    break;

                case 3:
                    _state.POVEast = true;
                    break;

                case 4:
                    _state.POVEast = _state.POVSouth = true;
                    break;

                case 5:
                    _state.POVSouth = true;
                    break;

                case 6:
                    _state.POVSouth = _state.POVWest = true;
                    break;

                case 7:
                    _state.POVWest = true;
                    break;

                case 8:
                    _state.POVWest = _state.POVNorth = true;
                    break;
            }

            return NotifyStateChanged();
        }
    }
}