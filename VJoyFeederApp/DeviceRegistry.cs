using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Microsoft.Win32;

namespace JoystickUsermodeDriver
{
    static class DeviceRegistry
    {
        public const string REGISTRY_KEY = "SOFTWARE\\BearBrains\\VirtualJoystick\\1.0";
        private const string ACTIVE_PROFILE_NAME = "ActiveProfileName";
        private const string DEVICE_NAME = "DeviceName";

        public const string REGISTRY_VALUE_VIRTUAL_DEVICE_TYPE = "virtualType";
        public const string REGISTRY_VALUE_VIRTUAL_DEVICE_INDEX = "virtualIndex";
        public const string REGISTRY_VALUE_SOURCE_TYPE = "sourceType";
        public const string REGISTRY_VALUE_SOURCE_INDEX = "sourceIndex";
        public const string REGISTRY_VALUE_INVERT = "invert";

        public static string GetActiveProfileName()
        {
            string activeProfile = null;
            using (RegistryKey k = Registry.CurrentUser.CreateSubKey(DeviceRegistry.REGISTRY_KEY))
            {
                activeProfile = k.GetValue(ACTIVE_PROFILE_NAME, "").ToString();
                if (activeProfile.Length == 0)
                {
                    return null;
                }
                else
                {
                    using (var profile = k.OpenSubKey(activeProfile))
                    {
                        if (profile == null)
                        {
                            return null;
                        }
                    }
                }
            }

            return activeProfile;
        }

        public static void GenerateDefaultProfile()
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
            }
        }

        private static void StoreSupportedAxes(RegistryKey k)
        {
            var enumType = typeof(VJoyDriverInterface.AxisIndex);
            foreach (var value in Enum.GetValues(enumType))
            {
                var name = Enum.GetName(enumType, value);
                k.SetValue($"AxisIndex: {name}", (Int32)value, RegistryValueKind.DWord);
            }
        }

        public static void StoreEnumeratedDevices(UInt32 driverHandle, List<DeviceDescription> devices)
        {
            using (RegistryKey k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY, true))
            {
                k.DeleteSubKeyTree("CurrentDevicePrototypes", false);
                var devicePrototypes = k.CreateSubKey("CurrentDevicePrototypes", true);

                StoreSupportedAxes(devicePrototypes);

                foreach (DeviceDescription d in devices)
                {
                    var deviceKey = devicePrototypes.CreateSubKey(d.GUID, true);
                    deviceKey.SetValue(DEVICE_NAME, d.displayName);

                    var axes = new List<Tuple<string, UInt32>>();
                    var buttons = new List<Tuple<string, UInt32>>();
                    var povs = new List<Tuple<string, UInt32>>();
                    VJoyDriverInterface.DeviceInfoCallback callback = (deviceType, name, objectIndex) =>
                    {
                        var data = new Tuple<string, UInt32>(name, objectIndex);
                        switch (deviceType)
                        {
                            case VJoyDriverInterface.MappingType.axis:
                                axes.Add(data);
                                break;
                            case VJoyDriverInterface.MappingType.button:
                                buttons.Add(data);
                                break;
                            case VJoyDriverInterface.MappingType.pov:
                                povs.Add(data);
                                break;
                        }
                    };

                    VJoyDriverInterface.GetDeviceInfo(driverHandle, d.GUID, callback);

                    deviceKey.SetValue("Axes", axes.Count);
                    deviceKey.SetValue("Buttons", buttons.Count);
                    deviceKey.SetValue("POVs", povs.Count);

                    // Generate pass-through mappings for ease of hand editing.


                    foreach (var item in axes)
                    {
                        var mapping = new VJoyDriverInterface.DeviceMapping(
                            VJoyDriverInterface.MappingType.axis,
                            item.Item2);
                        var subkeyName = $"Axis_{item.Item2}  {item.Item1}";
                        WriteEnumeratedDeviceMapping(subkeyName, deviceKey, mapping, item.Item1);
                    }

                    foreach (var item in buttons)
                    {
                        var mapping = new VJoyDriverInterface.DeviceMapping(
                            VJoyDriverInterface.MappingType.button,
                            item.Item2);
                        var subkeyName = $"Button_{item.Item2}  {item.Item1}";
                        WriteEnumeratedDeviceMapping(subkeyName, deviceKey, mapping, item.Item1);
                    }

                    foreach (var item in povs)
                    {
                        var mapping = new VJoyDriverInterface.DeviceMapping(
                            VJoyDriverInterface.MappingType.pov,
                            item.Item2);
                        var subkeyName = $"POV_{item.Item2}  {item.Item1}";
                        WriteEnumeratedDeviceMapping(subkeyName, deviceKey, mapping, item.Item1);
                    }
                }
            }
        }

        private static bool WriteEnumeratedDeviceMapping(
            string mappingKeyName,
            RegistryKey deviceKey,
            VJoyDriverInterface.DeviceMapping mapping,
            string mappingName)
        {
            mapping.WriteToRegistry(deviceKey, mappingKeyName);
            using (var mappingKey = deviceKey.CreateSubKey(mappingKeyName))
            {
                if (mappingKey == null)
                {
                    return false;
                }

                mappingKey.SetValue("sourceName", mappingName);
            }

            return true;
        }

        public static void LoadMappingsIntoDriver(UInt32 driverHandle)
        {
            VJoyDriverInterface.ClearDeviceMappings(driverHandle);
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

                LoadMappingsForProfile(driverHandle, k, activeProfileName.ToString());
            }
        }

        private static void LoadMappingsForProfile(UInt32 driverHandle, RegistryKey parentKey, string activeProfileName)
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
                    LoadMappingsForDevice(driverHandle, activeProfile, deviceID);
                }
            }
        }

        private static void LoadMappingsForDevice(UInt32 driverHandle, RegistryKey profileKey, string deviceID)
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

                VJoyDriverInterface.SetDeviceMapping(driverHandle, deviceID, mappings.ToArray(), mappings.Count);
            }
        }
    }

}
