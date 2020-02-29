using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using Microsoft.DirectX.DirectInput;
using Microsoft.Win32;


namespace JoystickUsermodeDriver
{
    public partial class MainFrame : Form
    {
        UInt32 _driverHandle; //!< Handle to the virtual joystick driver instance

        //! List of DeviceDescription instances for available physical devices
        List<DeviceDescription> _deviceEnumeration;

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
            var activeProfileName = DeviceRegistry.GetActiveProfileName();

            // If no joystick is configured, pop the chooser dialog, otherwise just start feeding
            if (activeProfileName == null)
            {
                DeviceRegistry.GenerateDefaultProfile();
                //DisplayJoystickChooser();
            }

            BeginFeedingDriver();
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
                WindowState = FormWindowState.Minimized;
            }
            else
            {
                MenuShow.Text = "Hide";
                WindowState = FormWindowState.Normal;
                this.DisplayJoystickGUIDs();
            }
        }


        private void MainFrame_FormClosed(object sender, FormClosedEventArgs e)
        {
            VJoyDriverInterface.DetachFromVirtualJoystickDriver(_driverHandle);
            _driverHandle = VJoyDriverInterface.INVALID_HANDLE_VALUE;
        }


        private void chooseDevicesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            VJoyDriverInterface.EndDriverUpdateLoop(_driverHandle);

            // The joystick chooser will start up the loop again
            DisplayJoystickChooser();
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

        private void DisplayJoystickGUIDs()
        {
            this.joystickDeviceList.Items.Clear();

            foreach (DeviceDescription d in this.DeviceEnumeration)
            {
                String[] data = new string[2];
                data[0] = d.displayName;
                data[1] = d.GUID;
                this.joystickDeviceList.Items.Add(new ListViewItem(data));
            }

            this.joystickDeviceList.Columns[0].Width = -1;
            this.joystickDeviceList.Columns[1].Width = -1;
        }

        private void DisplayJoystickChooser()
        {
            // Display the chooser dialog
            if (this.DeviceEnumeration.Count > 0)
            {
                DevicePicker dpDlg = new DevicePicker();
                dpDlg.populateLists(_deviceEnumeration);
                dpDlg.ShowDialog();

                // If the user clicked OK, update our devices
                if (dpDlg.isOK)
                {
                    // RegistryKey k = Registry.CurrentUser.OpenSubKey(REGISTRY_KEY, true);
                    // k.Close();

                    // Notify the driver of the update
                    BeginFeedingDriver();
                }
            }
            else
            {
                MessageBox.Show("Failed to find any physical joystick device!",
                    "Joysticks Not Found Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
            }
        }

        private void BeginFeedingDriver()
        {
            if (_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE)
                return;

            this.LoadMappingsIntoDriver();
            if (!VJoyDriverInterface.BeginDriverUpdateLoop(_driverHandle))
            {
                MessageBox.Show("Failed to interface with virtual joystick device! Code: 1",
                    "Interface Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
            }
        }

        private void LoadMappingsIntoDriver()
        {
            if (_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE)
                return;
            DeviceRegistry.LoadMappingsIntoDriver(_driverHandle);
        }




        private void StopFeedingDriver()
        {
            if (_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE)
                return;

            VJoyDriverInterface.EndDriverUpdateLoop(_driverHandle);
        }

        private void joystickDeviceList_ItemSelectionChanged(object sender, ListViewItemSelectionChangedEventArgs e)
        {
            var builder = new StringBuilder();
            foreach (ListViewItem item in this.joystickDeviceList.SelectedItems)
                builder.AppendLine(item.SubItems[1].Text);
            if (builder.Length > 0)
            {
                Clipboard.SetText(builder.ToString());
            }
        }

        private void reloadActiveProfileToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // Cycling the driver will reload the current profile.
            StopFeedingDriver();
            BeginFeedingDriver();
        }

        private void refreshToolStripMenuItem_Click(object sender, EventArgs e)
        {
            StopFeedingDriver();
            _deviceEnumeration.Clear();
            DisplayJoystickGUIDs();
            BeginFeedingDriver();
        }
    }
}