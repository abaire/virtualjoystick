using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Microsoft.Win32;
using System.Runtime.InteropServices;


namespace JoystickUsermodeDriver
{
    public partial class MainFrame : Form
    {
        public const String REGISTRY_KEY = "SOFTWARE\\BearBrains\\VirtualJoystick\\1.0";
        private const String ACTIVE_PROFILE_NAME = "ActiveProfileName";
        private const String DEVICE_NAME = "DeviceName";

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

            StoreEnumeratedDevices();

            bool validProfile = true;
            using (RegistryKey k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY))
            {
                var activeProfile = k.GetValue(ACTIVE_PROFILE_NAME, "").ToString();
                if (activeProfile.Length == 0)
                {
                    validProfile = false;
                }
                else
                {
                    using (var profile = k.OpenSubKey(activeProfile))
                    {
                        if (profile == null)
                        {
                            validProfile = false;
                        }
                    }
                }
            }

            // If no joystick is configured, pop the chooser dialog, otherwise just start feeding
            if (!validProfile)
            {
                this.GenerateDefaultProfile();
                //DisplayJoystickChooser();
            }

            BeginFeedingDriver();
        }

        private void StoreEnumeratedDevices()
        {
            using (RegistryKey k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY, true))
            {
                var devicePrototypes = k.CreateSubKey("CurrentDevicePrototypes", true);

                foreach (DeviceDescription d in this.DeviceEnumeration)
                {
                    var deviceKey = devicePrototypes.CreateSubKey(d.GUID, true);
                    deviceKey.SetValue(DEVICE_NAME, d.displayName);

                    UInt32 numAxes = 0;
                    UInt32 numButtons = 0;
                    UInt32 numPOVs = 0;
                    VJoyDriverInterface.GetDeviceInfo(
                        _driverHandle,
                        d.GUID,
                        ref numAxes,
                        ref numButtons,
                        ref numPOVs);

                    deviceKey.SetValue("Axes", numAxes);
                    deviceKey.SetValue("Buttons", numButtons);
                    deviceKey.SetValue("POVs", numPOVs);

                    // Generate pass-through mappings for ease of hand editing.
                    var index = 0;
                    for (UInt32 i = 0; i < numAxes; ++i)
                    {
                        var mapping = new VJoyDriverInterface.DeviceMapping(
                            VJoyDriverInterface.MappingType.axis,
                            i);
                        mapping.WriteToRegistry(deviceKey, $"Mapping_{index++}");
                    }

                    for (UInt32 i = 0; i < numButtons; ++i)
                    {
                        var mapping = new VJoyDriverInterface.DeviceMapping(
                            VJoyDriverInterface.MappingType.button,
                            i);
                        mapping.WriteToRegistry(deviceKey, $"Mapping_{index++}");
                    }

                    for (UInt32 i = 0; i < numPOVs; ++i)
                    {
                        var mapping = new VJoyDriverInterface.DeviceMapping(
                            VJoyDriverInterface.MappingType.pov,
                            i);
                        mapping.WriteToRegistry(deviceKey, $"Mapping_{index++}");
                    }
                }
            }
        }

        private void GenerateDefaultProfile()
        {
            using (RegistryKey k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY, true))
            {
                var numProfiles = k.GetSubKeyNames().Length;

                k.SetValue(ACTIVE_PROFILE_NAME, "Profile_0");

                if (numProfiles > 0)
                {
                    // TODO: Pop a UI to allow the active profile to be selected.
                    return;
                }

                // TODO: Pop a UI to allow joysticks to be enumerated and mapped.
                var profile = k.CreateSubKey("Profile_0", true);

                // var xbox_controller = profile.CreateSubKey("63C903B059A711EA8001444553540000");
                //
                // // xbox_controller.SetValue("DEVICE_NAME", "Controller (XBOX 360 For Windows)");
                // var index = 0;
                //
                // var mapping = new VJoyDriverInterface.DeviceMapping(
                //     VJoyDriverInterface.MappingType.axis,
                //     VJoyDriverInterface.AxisIndex.axis_x);
                // mapping.WriteToRegistry(xbox_controller, $"Mapping_{index++}");
                //
                // mapping = new VJoyDriverInterface.DeviceMapping(
                //     VJoyDriverInterface.MappingType.axis,
                //     VJoyDriverInterface.AxisIndex.axis_y);
                // mapping.WriteToRegistry(xbox_controller, $"Mapping_{index++}");
                //
                // mapping = new VJoyDriverInterface.DeviceMapping(
                //     VJoyDriverInterface.MappingType.axis,
                //     VJoyDriverInterface.AxisIndex.axis_throttle);
                // mapping.WriteToRegistry(xbox_controller, $"Mapping_{index++}");
                //
                // mapping = new VJoyDriverInterface.DeviceMapping(
                //     VJoyDriverInterface.MappingType.axis,
                //     VJoyDriverInterface.AxisIndex.axis_rx);
                // mapping.WriteToRegistry(xbox_controller, $"Mapping_{index++}");
                //
                // mapping = new VJoyDriverInterface.DeviceMapping(
                //     VJoyDriverInterface.MappingType.axis,
                //     VJoyDriverInterface.AxisIndex.axis_ry);
                // mapping.WriteToRegistry(xbox_controller, $"Mapping_{index++}");
                //
                // mapping = new VJoyDriverInterface.DeviceMapping(
                //     VJoyDriverInterface.MappingType.pov,
                //     (UInt32) 0);
                // mapping.WriteToRegistry(xbox_controller, $"Mapping_{index++}");
                //
                // for (UInt32 i = 0; i < 10; ++i)
                // {
                //     mapping = new VJoyDriverInterface.DeviceMapping(
                //         VJoyDriverInterface.MappingType.button,
                //         i);
                //     mapping.WriteToRegistry(xbox_controller, $"Mapping_{index++}");
                // }
            }
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
            VJoyDriverInterface.ClearDeviceMappings(_driverHandle);

            using (RegistryKey k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY))
            {
                var activeProfileName = k.GetValue(ACTIVE_PROFILE_NAME);
                if (activeProfileName == null)
                {
                    MessageBox.Show(
                        $"{ACTIVE_PROFILE_NAME} not set under {REGISTRY_KEY}. No mappings loaded.",
                        "Registry Error",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Exclamation);
                    return;
                }

                this.LoadMappingsForProfile(k, activeProfileName.ToString());
            }
        }

        private void LoadMappingsForProfile(RegistryKey parentKey, string activeProfileName)
        {
            using (RegistryKey activeProfile = parentKey.OpenSubKey(activeProfileName))
            {
                if (activeProfile == null)
                {
                    MessageBox.Show(
                        $"{activeProfileName} does not exist under {REGISTRY_KEY}. No mappings loaded.",
                        "Registry Error",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Exclamation);
                    return;
                }

                foreach (var deviceID in activeProfile.GetSubKeyNames())
                {
                    this.LoadMappingsForDevice(activeProfile, deviceID);
                }
            }
        }

        private void LoadMappingsForDevice(RegistryKey profileKey, string deviceID)
        {
            using (RegistryKey device = profileKey.OpenSubKey(deviceID))
            {
                if (device == null)
                {
                    MessageBox.Show(
                        $"{deviceID} does not exist in profile {profileKey.Name}. Skipping device.",
                        "Registry Error",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Exclamation);
                    return;
                }

                var mappings = new List<VJoyDriverInterface.DeviceMapping>();
                foreach (var mappingName in device.GetSubKeyNames())
                {
                    using (RegistryKey mappingKey = device.OpenSubKey(mappingName))
                    {
                        if (mappingKey == null)
                        {
                            MessageBox.Show(
                                $"{mappingName} does not exist in device {deviceID} in profile {profileKey.Name}. Skipping device.",
                                "Registry Error",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Exclamation);
                            return;
                        }

                        var m = new VJoyDriverInterface.DeviceMapping(mappingKey);
                        mappings.Add(m);
                    }
                }

                VJoyDriverInterface.SetDeviceMapping(_driverHandle, deviceID, mappings.ToArray(), mappings.Count);
            }
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