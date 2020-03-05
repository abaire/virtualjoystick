using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace JoystickUsermodeDriver
{
    public partial class ProfileEditorPanel : TableLayoutPanel
    {
        public ProfileEditorPanel()
        {
            GrowStyle = TableLayoutPanelGrowStyle.AddRows;
            RowCount = 0;
            ColumnCount = 3;
            ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 6F));
            ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 33F));
            ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 35F));
            BorderStyle = BorderStyle.FixedSingle;

            InitializeComponent();
        }

        public void Clear()
        {
            Controls.Clear();
            RowStyles.Clear();
            RowCount = 0;
        }

        public void SetProfile(
            SortedDictionary<string, List<VJoyDriverInterface.DeviceMapping>> profile,
            List<DeviceDescription> deviceEnumeration)
        {
            Clear();
            Func<string, string> GetDeviceName = (deviceID) =>
            {
                foreach (var dd in deviceEnumeration)
                {
                    if (dd.GUID == deviceID)
                    {
                        return dd.displayName;
                    }
                }

                return deviceID;
            };

            foreach (var deviceMapping in profile)
            {
                var deviceID = deviceMapping.Key;
                var mappings = deviceMapping.Value;

                var deviceLabel = new Label();
                deviceLabel.Text = GetDeviceName(deviceID);
                deviceLabel.Dock = DockStyle.Fill;
                Controls.Add(deviceLabel, 0, RowCount++);
                SetColumnSpan(deviceLabel, 3);

                foreach (var mapping in mappings)
                {
                    AddMapping(mapping);
                }

            }
        }

        private void AddMapping(VJoyDriverInterface.DeviceMapping mapping)
        {
            var sourceLabel = new Label();
            sourceLabel.Text = mapping.SourceName;
            Controls.Add(sourceLabel, 1, RowCount);

            var targetSelector = CreateComboBox(mapping.SourceType);
            targetSelector.Text = mapping.VirtualDeviceName;
            Controls.Add(targetSelector, 2, RowCount);

            ++RowCount;
        }

        private ComboBox CreateComboBox(VJoyDriverInterface.MappingType type)
        {
            var ret = new ComboBox();
            switch (type)
            {
                case VJoyDriverInterface.MappingType.Axis:
                    foreach (var item in Enum.GetNames(typeof(VJoyDriverInterface.AxisIndex)))
                    {
                        ret.Items.Add(item);
                    }
                    break;

                case VJoyDriverInterface.MappingType.Button:
                    for (var i = 0; i < VJoyDriverInterface.MaxVirtualButtons; ++i)
                    {
                        ret.Items.Add($"Button {i + 1}");
                    }
                    break;

                case VJoyDriverInterface.MappingType.POV:
                    for (var i = 0; i < VJoyDriverInterface.MaxVirtualPOVs; ++i)
                    {
                        ret.Items.Add($"POV {i + 1}");
                    }

                    // POVs can also be mapped to 4-button ranges.
                    for (var i = 0; i < VJoyDriverInterface.MaxVirtualButtons - 4; ++i)
                    {
                        ret.Items.Add($"Buttons {i + 1} - {i + 4}");
                    }

                    break;
            }

            return ret;
        }
    }
}
