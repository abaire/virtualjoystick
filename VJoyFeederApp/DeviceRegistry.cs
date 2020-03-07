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
        private const string CURRENT_DEVICE_PROTOTYPES = "CurrentDevicePrototypes";

        public const string REGISTRY_VALUE_UPDATE_LOOP_DELAY = "updateLoopDelay";

        public const string REGISTRY_VALUE_VIRTUAL_DEVICE_TYPE = "virtualType";
        public const string REGISTRY_VALUE_VIRTUAL_DEVICE_INDEX = "virtualIndex";
        public const string REGISTRY_VALUE_SOURCE_TYPE = "sourceType";
        public const string REGISTRY_VALUE_SOURCE_INDEX = "sourceIndex";
        public const string REGISTRY_VALUE_INVERT = "invert";

        public static UInt32 UpdateLoopDelayMillis
        {
            get
            {
                using (RegistryKey k = Registry.CurrentUser.CreateSubKey(DeviceRegistry.REGISTRY_KEY))
                {
                    return Convert.ToUInt32(k.GetValue(REGISTRY_VALUE_UPDATE_LOOP_DELAY, 0));
                }
            }

            set
            {
                using (RegistryKey k = Registry.CurrentUser.CreateSubKey(DeviceRegistry.REGISTRY_KEY))
                {
                    k.SetValue(REGISTRY_VALUE_UPDATE_LOOP_DELAY, value, RegistryValueKind.DWord);
                }
            }
        }

        public static List<string> GetProfiles()
        {
            var ret = new List<string>();
            using (RegistryKey k = Registry.CurrentUser.CreateSubKey(DeviceRegistry.REGISTRY_KEY))
            {
                foreach (var profileName in k.GetSubKeyNames())
                {
                    if (profileName == CURRENT_DEVICE_PROTOTYPES)
                    {
                        continue;
                    }

                    ret.Add(profileName);
                }
            }

            return ret;
        }

        public static string ActiveProfileName
        {
            get
            {
                string activeProfile = null;
                using (RegistryKey k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY))
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

            set
            {
                using (RegistryKey k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY, true))
                {
                    k.SetValue(ACTIVE_PROFILE_NAME, value);
                }
            }
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
                k.SetValue($"AxisIndex: {name}", (Int32) value, RegistryValueKind.DWord);
            }
        }

        public static void StoreEnumeratedDevices(UInt32 driverHandle, List<DeviceDescription> devices)
        {
            using (RegistryKey k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY, true))
            {
                k.DeleteSubKeyTree(CURRENT_DEVICE_PROTOTYPES, false);
                var devicePrototypes = k.CreateSubKey(CURRENT_DEVICE_PROTOTYPES, true);

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
                            case VJoyDriverInterface.MappingType.Axis:
                                axes.Add(data);
                                break;
                            case VJoyDriverInterface.MappingType.Button:
                                buttons.Add(data);
                                break;
                            case VJoyDriverInterface.MappingType.POV:
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
                            VJoyDriverInterface.MappingType.Axis,
                            item.Item2);
                        var subkeyName = $"Axis_{item.Item2}  {item.Item1}";
                        WriteEnumeratedDeviceMapping(subkeyName, deviceKey, mapping, item.Item1);
                    }

                    foreach (var item in buttons)
                    {
                        var mapping = new VJoyDriverInterface.DeviceMapping(
                            VJoyDriverInterface.MappingType.Button,
                            item.Item2);
                        var subkeyName = $"Button_{item.Item2}  {item.Item1}";
                        WriteEnumeratedDeviceMapping(subkeyName, deviceKey, mapping, item.Item1);
                    }

                    foreach (var item in povs)
                    {
                        var mapping = new VJoyDriverInterface.DeviceMapping(
                            VJoyDriverInterface.MappingType.POV,
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

        public static SortedDictionary<string, List<VJoyDriverInterface.DeviceMapping>> LoadActiveProfileMappings()
        {
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
                    return null;
                }

                return LoadMappings(activeProfileName.ToString());
            }
        }

        public static SortedDictionary<string, List<VJoyDriverInterface.DeviceMapping>> LoadMappings(string profileName)
        {
            using (RegistryKey k = Registry.CurrentUser.CreateSubKey(REGISTRY_KEY))
            {
                return LoadMappingsForProfile(k, profileName);
            }
        }

        private static SortedDictionary<string, List<VJoyDriverInterface.DeviceMapping>> LoadMappingsForProfile(
            RegistryKey parentKey,
            string profileName)
        {
            using (RegistryKey activeProfile = parentKey.OpenSubKey(profileName))
            {
                if (activeProfile == null)
                {
                    MessageBox.Show(
                        $"{profileName} does not exist under {REGISTRY_KEY}. No mappings loaded.",
                        "Registry Error",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Exclamation);
                    return null;
                }

                var deviceToMappings = new SortedDictionary<string, List<VJoyDriverInterface.DeviceMapping>>();
                foreach (var deviceID in activeProfile.GetSubKeyNames())
                {
                    var mappings = LoadMappingsForDevice(activeProfile, deviceID);
                    if (mappings != null)
                    {
                        deviceToMappings[deviceID] = mappings;
                    }
                }

                return deviceToMappings;
            }
        }

        private static List<VJoyDriverInterface.DeviceMapping> LoadMappingsForDevice(RegistryKey profileKey,
            string deviceID)
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
                    return null;
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
                            return null;
                        }

                        try
                        {
                            var m = new VJoyDriverInterface.DeviceMapping(mappingKey);
                            mappings.Add(m);
                        }
                        catch (ArgumentException e)
                        {
                            MessageBox.Show(
                                $"Failed to parse {mappingName} in device {deviceID} in profile {profileKey.Name}. Skipping.",
                                "Registry Error",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Exclamation);
                        }
                    }
                }

                return mappings;
            }
        }
    }
}