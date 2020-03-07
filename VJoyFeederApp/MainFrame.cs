using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Windows.Forms;


namespace JoystickUsermodeDriver
{
    public partial class MainFrame : Form, IVirtualDeviceStateWatcher
    {
        private UInt32 _driverHandle; //!< Handle to the virtual joystick driver instance
        private bool _driverFeedRunning = false;

        //! List of DeviceDescription instances for available physical devices
        private List<DeviceDescription> _deviceEnumeration;

        private TCPController _tcpController;

        public MainFrame()
        {
            InitializeComponent();

            _deviceEnumeration = new List<DeviceDescription>();

            _driverHandle = VJoyDriverInterface.AttachToVirtualJoystickDriver();
            if (_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE)
            {
                MessageBox.Show("Failed to find virtual joystick device!",
                    "Driver Not Found Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }

            DeviceRegistry.StoreEnumeratedDevices(this._driverHandle, this.DeviceEnumeration);
            var activeProfileName = DeviceRegistry.ActiveProfileName;

            // If no joystick is configured, pop the chooser dialog, otherwise just start feeding
            if (activeProfileName == null)
            {
                DeviceRegistry.GenerateDefaultProfile();
            }

            RefreshDisplay();
            BeginFeedingDriver();

            _tcpController = new TCPController();
            _tcpController.StateDelegate = this;
            _tcpController.Start();

#if DEBUG
            MenuShow_Click(null, null);
#endif
        }

        private void PopulateProfileList()
        {
            var selectedItem = DeviceRegistry.ActiveProfileName;
            profileList.SelectedIndexChanged -= profileList_SelectedIndexChanged;
            profileList.DataSource = DeviceRegistry.GetProfiles();
            profileList.SelectedItem = selectedItem;
            profileList.SelectedIndexChanged += profileList_SelectedIndexChanged;
        }

        void IVirtualDeviceStateWatcher.StateUpdated(in VJoyDriverInterface.VirtualDeviceState state)
        {
            VJoyDriverInterface.SetVirtualDeviceState(_driverHandle, state);
        }

        private void MenuClose_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void MenuShow_Click(object sender, EventArgs e)
        {
            if (WindowState == FormWindowState.Normal)
            {
                MenuShow.Text = "Show";
                Hide();
                WindowState = FormWindowState.Minimized;
            }
            else
            {
                MenuShow.Text = "Hide";
                Show();
                WindowState = FormWindowState.Normal;
                RefreshDisplay();
            }
        }


        private void MainFrame_FormClosed(object sender, FormClosedEventArgs e)
        {
            if (_tcpController != null)
            {
                _tcpController.Stop();
            }

            VJoyDriverInterface.DetachFromVirtualJoystickDriver(_driverHandle);
            _driverHandle = VJoyDriverInterface.INVALID_HANDLE_VALUE;
        }

        public void deviceEnumCallback(string name, string guid)
        {
            DeviceDescription dd;
            dd.displayName = name;
            dd.GUID = guid;
            _deviceEnumeration.Add(dd);
        }

        public List<DeviceDescription> DeviceEnumeration
        {
            get
            {
                if (_deviceEnumeration.Count == 0)
                {
                    if (!VJoyDriverInterface.EnumerateDevices(_driverHandle,
                        new VJoyDriverInterface.DeviceEnumCallback(deviceEnumCallback)))
                    {
                        MessageBox.Show("Attempt to search for physical joystick devices failed!",
                            "Interface Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Exclamation);
                    }
                }

                return _deviceEnumeration;
            }
        }

        private string GetDeviceName(string deviceID)
        {
            foreach (var device in DeviceEnumeration)
            {
                if (device.GUID == deviceID)
                {
                    return device.displayName;
                }
            }

            return null;
        }

        private void PopulateProfileDisplay()
        {
            var selectedProfile = profileList.SelectedItem;
            if (selectedProfile == null)
            {
                _profileEditor.Clear();
                return;
            }
            var profile = DeviceRegistry.LoadMappings(selectedProfile.ToString());
            _profileEditor.SetProfile(profile, DeviceEnumeration);
        }

        private void BeginFeedingDriver()
        {
            if (_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE || _driverFeedRunning)
                return;

            this.LoadMappingsIntoDriver();
            if (!VJoyDriverInterface.BeginDriverUpdateLoop(_driverHandle))
            {
                MessageBox.Show("Failed to interface with virtual joystick device! Code: 1",
                    "Interface Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
            }
            _driverFeedRunning = true;
        }

        private void LoadMappingsIntoDriver()
        {
            if (_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE)
                return;

            VJoyDriverInterface.ClearDeviceMappings(_driverHandle);

            var profile = DeviceRegistry.LoadActiveProfileMappings();
            foreach (var deviceMapping in profile)
            {
                var deviceID = deviceMapping.Key;
                var mappings = deviceMapping.Value;
                VJoyDriverInterface.SetDeviceMapping(_driverHandle, deviceID, mappings.ToArray(), mappings.Count);
            }
        }

        private void StopFeedingDriver()
        {
            if (_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE || !_driverFeedRunning)
                return;

            VJoyDriverInterface.EndDriverUpdateLoop(_driverHandle);
            _driverFeedRunning = false;
        }

        private void ReloadActiveProfile()
        {
            // Cycling the driver will reload the current profile.
            StopFeedingDriver();
            BeginFeedingDriver();
        }

        private void reloadActiveProfileToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ReloadActiveProfile();
        }

        private void RefreshDisplay()
        {
            PopulateProfileList();
            PopulateProfileDisplay();
        }

        private void refreshToolStripMenuItem_Click(object sender, EventArgs e)
        {
            StopFeedingDriver();
            _deviceEnumeration.Clear();
            RefreshDisplay();
            BeginFeedingDriver();
        }

        private void profileList_SelectedIndexChanged(object sender, EventArgs e)
        {
            StopFeedingDriver();
            var activeProfileName = (string) profileList.SelectedItem;
            DeviceRegistry.ActiveProfileName = activeProfileName;
            RefreshDisplay();
            BeginFeedingDriver();
        }
    }
}