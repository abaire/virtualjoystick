using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml.Schema;

namespace JoystickUsermodeDriver
{
    internal interface ICloseDelegate
    {
        void ControlClientClosed(ControlClient client);
    }

    public class TCPController: ICloseDelegate
    {
        private const int MaxClients = 10;

        public string Address;

        private short _port = 13057;
        private TcpListener _listener;
        private List<ControlClient> _clients = new List<ControlClient>(MaxClients);

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
            var self = ((TCPController) ar.AsyncState);
            if (self._listener == null)
            {
                return;
            }

            TcpClient client = self._listener.EndAcceptTcpClient(ar);

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

        void ICloseDelegate.ControlClientClosed(ControlClient client)
        {
            _clients.Remove(client);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal class Handshake
    {
        internal byte[] magicBytes = Encoding.ASCII.GetBytes("bbvj");
        internal byte version;
    }

    internal class ControlProtocol
    {
        enum State
        {
            WaitingForHandshake,
            AcceptingCommands
        }

        private State _state;

        internal bool Handle(byte[] buffer, ref int bufferLength)
        {
            bool result;
            switch (_state)
            {
                case State.WaitingForHandshake:
                    result = HandleHandshake(buffer, ref bufferLength);
                    break;

                case State.AcceptingCommands:
                    result = HandleHandshake(buffer, ref bufferLength);
                    break;

                default:
                    result = false;
                    break;
            }

            return result;
        }

        private bool HandleHandshake(byte[] buffer, ref int bufferLength)
        {
            return false;
        }

        private void ShiftBuffer(byte[] buffer, int bytesConsumed, ref int bufferLength)
        {
            if (bufferLength <= bytesConsumed)
            {
                bufferLength = 0;
                return;
            }

            var bytesToMove = bufferLength - bytesConsumed;
            Buffer.BlockCopy(buffer, bytesConsumed, buffer, 0, bytesToMove);
        }
    }

    internal class ControlClient
    {
        private const int BufferSize = 256;
        private TcpClient _client;
        private byte[] _buffer;
        private int _bufferWriteHead = 0;
        private ControlProtocol _protocol;
        private WeakReference<ICloseDelegate> _closeDelegate;

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
            if (_closeDelegate.TryGetTarget(out d))
            {
                d.ControlClientClosed(this);
            }
        }

        private bool HandleData()
        {
            if (!_protocol.Handle(_buffer, ref _bufferWriteHead))
            {
                _client.Close();
                NotifyClosed();
                return false;
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
                int bytesRead = stream.EndRead(ar);
                if (bytesRead == 0)
                {
                    self.NotifyClosed();
                    return;
                }

                self._bufferWriteHead += bytesRead;
                if (!self.HandleData())
                {
                    return;
                }

                stream.BeginRead(self._buffer, self._bufferWriteHead, BufferSize - self._bufferWriteHead, ReadData, self);
            }
            catch (IOException)
            {
                self.NotifyClosed();
                return;
            }
            catch (ObjectDisposedException)
            {
                self.NotifyClosed();
                return;
            }
        }
    }
}