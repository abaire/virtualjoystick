using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace JoystickUsermodeDriver
{
    public partial class DevicePicker : Form
    {
        public bool isOK;


        public DevicePicker()
        {
            InitializeComponent();
        }


        public void populateLists(List<DeviceDescription> devices)
        {
            foreach (DeviceDescription d in devices)
            {
                m_joystickDevice.Items.Add(d);
                m_rudderDevice.Items.Add(d);
            }

            if (m_joystickDevice.Items.Count > 0)
                m_joystickDevice.SelectedIndex = 0;
            if (m_rudderDevice.Items.Count > 1)
                m_rudderDevice.SelectedIndex = 1;
            else if (m_rudderDevice.Items.Count > 0)
                m_rudderDevice.SelectedIndex = 0;
        }


        public DeviceDescription joystickDevice
        {
            get { return (DeviceDescription) m_joystickDevice.SelectedItem; }
        }

        public DeviceDescription rudderDevice
        {
            get { return (DeviceDescription) m_rudderDevice.SelectedItem; }
        }

        private void okButton_Click(object sender, EventArgs e)
        {
            isOK = true;
            Close();
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            isOK = false;
            Close();
        }
    }
}