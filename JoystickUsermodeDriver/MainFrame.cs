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

        UInt32 m_driverHandle; //!< Handle to the virtual joystick driver instance
        string m_joystickGUID; //!< GUID of the physical device to be treated as a joystick
        string m_rudderGUID; //!< GUID of the physical device to be treated as a rudder

        List<DeviceDescription>
            m_deviceEnumeration; //!< List of DeviceDescription instances for available physical devices


        public MainFrame()
        {
            InitializeComponent();

            m_deviceEnumeration = new List<DeviceDescription>();

            // Try to find our driver
            m_driverHandle = VJoyDriverInterface.AttachToVirtualJoystickDriver();
            if (m_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE)
            {
                MessageBox.Show("Failed to find virtual joystick device!",
                    "Driver Not Found Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }

            // Check the registry to see if we have a device map set up
            RegistryKey k = Registry.CurrentUser.OpenSubKey(REGISTRY_KEY);
            if (k == null)
                k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY);

            m_joystickGUID = k.GetValue("JOYSTICK_GUID", "").ToString();
            m_rudderGUID = k.GetValue("RUDDER_GUID", "").ToString();
            k.Close();

            // If no joystick is configured, pop the chooser dialog, otherwise just start feeding
            if (m_joystickGUID.Length == 0)
                displayJoystickChooser();
            else
                beginFeedingDriver();
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
            }
        }


        private void MainFrame_FormClosed(object sender, FormClosedEventArgs e)
        {
            VJoyDriverInterface.DetachFromVirtualJoystickDriver(m_driverHandle);
            m_driverHandle = VJoyDriverInterface.INVALID_HANDLE_VALUE;
        }


        private void chooseDevicesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            VJoyDriverInterface.EndDriverUpdateLoop(m_driverHandle);

            // The joystick chooser will start up the loop again
            displayJoystickChooser();
        }


        public void deviceEnumCallback(string name, string guid)
        {
            DeviceDescription dd;
            dd.displayName = name;
            dd.GUID = guid;
            m_deviceEnumeration.Add(dd);
        }


        private void displayJoystickChooser()
        {
            if (m_deviceEnumeration.Count == 0)
            {
                if (!VJoyDriverInterface.EnumerateDevices(m_driverHandle,
                    new VJoyDriverInterface.DeviceEnumCallback(deviceEnumCallback)))
                {
                    MessageBox.Show("Attempt to search for physical joystick devices failed!",
                        "Interface Error",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Exclamation);
                    return;
                }
            }

            // Display the chooser dialog
            if (m_deviceEnumeration.Count > 0)
            {
                DevicePicker dpDlg = new DevicePicker();
                dpDlg.populateLists(m_deviceEnumeration);
                dpDlg.ShowDialog();

                // If the user clicked OK, update our devices
                if (dpDlg.isOK)
                {
                    RegistryKey k = Registry.CurrentUser.OpenSubKey(REGISTRY_KEY, true);

                    m_joystickGUID = dpDlg.joystickDevice.GUID;
                    m_rudderGUID = dpDlg.rudderDevice.GUID;

                    k.SetValue("JOYSTICK_GUID", m_joystickGUID);
                    k.SetValue("RUDDER_GUID", m_rudderGUID);
                    k.Close();

                    // Notify the driver of the update
                    beginFeedingDriver();
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


        private void beginFeedingDriver()
        {
            if (m_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE)
                return;

            // VJoyDriverInterface.SetDeviceIDs(m_driverHandle, m_joystickGUID, m_rudderGUID);
            if (!VJoyDriverInterface.BeginDriverUpdateLoop(m_driverHandle))
            {
                MessageBox.Show("Failed to interface with virtual joystick device! Code: 1",
                    "Interface Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
            }
        }


        private void stopFeedingDriver()
        {
            if (m_driverHandle == VJoyDriverInterface.INVALID_HANDLE_VALUE)
                return;

            VJoyDriverInterface.EndDriverUpdateLoop(m_driverHandle);
        }

        private void listDeviceGUIDsToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }
    }
}