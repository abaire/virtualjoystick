namespace JoystickUsermodeDriver
{
    //! \struct DeviceDescription
    //! \brief  Simple wrapper struct that links a direct input device to its friendly name
    public struct DeviceDescription
    {
        public string displayName; //!< The friendly name of this device
        public string GUID; //!< The GUID of this device

        public override string ToString()
        {
            return displayName;
        }
    };
}