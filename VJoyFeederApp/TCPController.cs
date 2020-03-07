using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;

namespace JoystickUsermodeDriver
{
    internal interface ICloseDelegate
    {
        void ControlClientClosed(ControlClient client);
    }

    public class TCPController : ICloseDelegate, IControlProtocolDelegate
    {
        private const int MaxClients = 10;
        private readonly List<ControlClient> _clients = new List<ControlClient>(MaxClients);

        private readonly short _port = 13057;
        private TcpListener _listener;

        public string Address;

        void ICloseDelegate.ControlClientClosed(ControlClient client)
        {
            _clients.Remove(client);
        }

        bool IControlProtocolDelegate.HandleKeycode(uint keycode, bool isPressed)
        {
            return true;
        }

        public void Start()
        {
            Stop();

            IPAddress addr;
            if (Address != null)
            {
                addr = IPAddress.Parse(Address);
            }
            else
            {
                var entry = Dns.GetHostEntry("localhost");
                addr = entry.AddressList[0];
            }

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
                var controller = new ControlClient(client, self);
                self._clients.Add(controller);
                controller.Start();
            }

            self._listener.BeginAcceptTcpClient(AcceptClient, self);
        }
    }

    internal class ControlClient
    {
        private const int BufferSize = 256;
        private readonly TcpClient _client;
        private readonly WeakReference<ICloseDelegate> _closeDelegate;
        private readonly ControlProtocol _protocol;
        private byte[] _buffer;
        private int _bufferWriteHead;

        internal ControlClient(TcpClient client, ICloseDelegate closeDelegate)
        {
            _client = client;
            _buffer = new byte[BufferSize];
            _protocol = new ControlProtocol();
            _closeDelegate = new WeakReference<ICloseDelegate>(closeDelegate);
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

        private void NotifyClosed()
        {
            ICloseDelegate d;
            if (_closeDelegate.TryGetTarget(out d)) d.ControlClientClosed(this);
        }

        private bool HandleData()
        {
            var previousBytesAvailable = -1;
            while (_bufferWriteHead != previousBytesAvailable)
            {
                previousBytesAvailable = _bufferWriteHead;
                if (!_protocol.Handle(ref _buffer, ref _bufferWriteHead))
                {
                    _client.Close();
                    NotifyClosed();
                    return false;
                }

                // If a command was consumed, send an ack reply.
                if (_bufferWriteHead != previousBytesAvailable) _client.GetStream().WriteByte(0x01);
            }

            return true;
        }

        private static void ReadData(IAsyncResult ar)
        {
            var self = (ControlClient) ar.AsyncState;
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

                stream.BeginRead(self._buffer, self._bufferWriteHead, BufferSize - self._bufferWriteHead, ReadData,
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
    }
}